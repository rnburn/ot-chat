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

  if (req_.method() != http::verb::get && req_.method() != http::verb::head) {
    res_ = http::response<http::string_body>{http::status::bad_request,
                                             req_.version()};
    res_.body() = "Unknown HTTP-method";
    res_.content_length(res_.body().size());
  } else if (req_.target() != "/") {
    res_ = http::response<http::string_body>{http::status::bad_request,
                                             req_.version()};
    res_.body() = "Illegal request target";
    res_.content_length(res_.body().size());
  } else if (req_.method() == http::verb::head) {
    res_ = http::response<http::string_body>{http::status::ok, req_.version()};
    res_.content_length(config_.html.size());
  } else {
    res_ = http::response<http::string_body>{http::status::ok, req_.version()};
    res_.body() = config_.html;
    res_.content_length(res_.body().size());
  }

  res_.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res_.set(http::field::content_type, "text/html");
  res_.keep_alive(req_.keep_alive());

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
