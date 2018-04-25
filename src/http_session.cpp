#include "http_session.h"

#include "chat_session.h"
#include "tracing.h"

#include <opentracing/ext/tags.h>
#include <cstring>
#include <iostream>
#include <type_traits>

namespace ot_chat {
static void set_request_tags(const http::request<http::string_body>& req,
                             opentracing::Span& span) {
  auto method_str = http::to_string(req.method());
  span.SetTag(opentracing::ext::http_method,
              opentracing::string_view{method_str.data(), method_str.size()});
  auto target = req.target();
  span.SetTag(opentracing::ext::http_url,
              opentracing::string_view{target.data(), target.size()});
}

static http::response<http::span_body<const char>> make_response(
    const http::request<http::string_body>& req, http::status status,
    const char* content_type,
    boost::beast::span<const char> body) {
  http::response<http::span_body<const char>> res{status, req.version()};
  res.body() = body;
  res.content_length(body.size());
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, content_type);
  res.keep_alive(req.keep_alive());
  return res;
}

static http::response<http::span_body<const char>> make_response(
    const http::request<http::string_body>& req, http::status status,
    const char* content_type, const char* body) {
  return make_response(req, status, content_type, {body, std::strlen(body)});
}

static http::response<http::span_body<const char>> handle_request(
    const configuration& config, const http::request<http::string_body>& req) {
  if (req.method() != http::verb::get && req.method() != http::verb::head) {
    return make_response(req, http::status::bad_request, "text/html",
                         "Unknown HTTP-method");
  }

  auto target = req.target();
  if (target == "/")
    target = "/index.html";
  auto iter = config.resources.find(std::string{target});
  if (iter == config.resources.end())
    return make_response(req, http::status::bad_request, "text/html",
                         "Illegal request target");
  auto& resource = iter->second;
  auto res =
      make_response(req, http::status::ok, resource.content_type,
                    {resource.content.data(), resource.content.size()});
  if (req.method() == http::verb::head) {
    res.body() = {};
  }
  return res;
}

http_session::http_session(const configuration& config, chat_room& chat_room,
                           tcp::socket&& socket)
    : config_{config}, chat_room_{chat_room}, socket_{std::move(socket)} {}

void http_session::run() { do_read(); }

void http_session::do_read() {
  http::async_read(socket_, buffer_, req_,
                   [self = shared_from_this()](boost::system::error_code ec,
                                               std::size_t bytes_transferred) {
                     self->on_read(ec, bytes_transferred);
                   });
}

void http_session::on_read(boost::system::error_code ec,
                           std::size_t bytes_transferred) {
  auto read_span = config_.tracer->StartSpan("ReadHttp");
  set_request_tags(req_, *read_span);

  if (ec == http::error::end_of_stream) return do_close();

  if (ec) {
    config_.logger->info("read failed: {}", ec.message());
    set_span_error_code(ec, *read_span);
    return;
  }

  if (websocket::is_upgrade(req_)) {
    read_span->SetOperationName("WebSocketUpgradeRequest");
    std::make_shared<chat_session>(config_, chat_room_, std::move(socket_))
        ->run(read_span->context(), std::move(req_));
    return;
  }

  res_ = handle_request(config_, req_);

  auto write_span = config_.tracer->StartSpan(
      "WriteHttp", {opentracing::FollowsFrom(&read_span->context())});
  write_span->SetTag(opentracing::ext::http_status_code, res_.result_int());

  http::async_write(socket_, res_, [
    self = this->shared_from_this(),
    write_span = std::shared_ptr<opentracing::Span>{write_span.release()}
  ](boost::system::error_code ec, std::size_t bytes_transferred) {
    self->on_write(*write_span, ec, bytes_transferred, self->res_.need_eof());
  });
}

void http_session::do_close() {
  // Send a TCP shutdown
  boost::system::error_code ec;
  socket_.shutdown(tcp::socket::shutdown_send, ec);

  // At this point the connection is closed gracefully
}

void http_session::on_write(opentracing::Span& write_span,
                            boost::system::error_code ec,
                            std::size_t bytes_transferred, bool close) {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    config_.logger->info("write failed: {}", ec.message());
    set_span_error_code(ec, write_span);
    return;
  }

  if (close) {
    // This means we should close the connection, usually because
    // the response indicated the "Connection: close" semantic.
    return do_close();
  }

  write_span.Finish();

  // Read another request
  do_read();
}
}  // namespace ot_chat
