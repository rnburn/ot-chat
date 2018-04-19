#include "chat_session.h"

#include "tracing.h"

#include <cassert>
#include <opentracing/ext/tags.h>

namespace ot_chat {
chat_session::chat_session(const configuration& config, 
    chat_room& chat_room,
    tcp::socket&& socket) 
  : config_{config}, chat_room_{chat_room}, ws_{std::move(socket)}
{}

void chat_session::run(const opentracing::SpanContext& read_context,
                       http::request<http::string_body>&& req) {
  auto span = config_.tracer->StartSpan(
      "AcceptWebSocket", {opentracing::FollowsFrom(&read_context)});
  ws_.async_accept(req, [
    self = shared_from_this(),
    span = std::shared_ptr<opentracing::Span>{span.release()}
  ](boost::system::error_code ec) { self->on_accept(*span, ec); });
}

void chat_session::on_accept(opentracing::Span& span,
                             boost::system::error_code ec) {
  if (ec) {
    config_.logger->info("accept failed: {}", ec.message());
    set_span_error_code(ec, span);
    return;
  }
  chat_room_.enter(this);
  span.Finish();
  do_read();
}

void chat_session::do_read() {
  auto buffer = std::make_shared<boost::beast::multi_buffer>();
  ws_.async_read(*buffer, [
    self = shared_from_this(), buffer = buffer
  ](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
    self->on_read(ec,  buffer);
  });
}

void chat_session::do_write(
    const std::shared_ptr<opentracing::Span>& span,
    const std::shared_ptr<const boost::beast::multi_buffer>& buffer) {
  ws_.async_write(buffer->data(), [
    self = shared_from_this(), span = span, buffer = buffer
  ](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
    self->on_write(*span, ec);
  });
}

void chat_session::on_read(boost::system::error_code ec,
    const std::shared_ptr<boost::beast::multi_buffer>& buffer) {
  auto span = config_.tracer->StartSpan("ReceiveMsg");
  if (ec == websocket::error::closed) {
    chat_room_.leave(this);
    return;
  }

  if (ec) {
    config_.logger->info("read failed: {}", ec.message());
    set_span_error_code(ec, *span);
  }

  chat_room_.broadcast(&span->context(), buffer);
  span->Finish();
  do_read();
}

void chat_session::send(
    const opentracing::SpanContext* parent_span_context,
    const std::shared_ptr<const boost::beast::multi_buffer>& message) {
  std::lock_guard<std::mutex> lock_guard_{write_mutex_};
  std::shared_ptr<opentracing::Span> span{config_.tracer->StartSpan(
      "WriteMsg", {opentracing::FollowsFrom(parent_span_context)})};
  if (write_in_progress_) {
    message_queue_.push(WriteEvent{span, message});
    return;
  }
  assert(message_queue_.empty());
  write_in_progress_ = true;
  do_write(span, message);
}

void chat_session::on_write(opentracing::Span& span, boost::system::error_code ec) {
  std::lock_guard<std::mutex> lock_guard_{write_mutex_};
  write_in_progress_ = false;
  if (ec) {
    config_.logger->info("write failed: {}", ec.message());
    set_span_error_code(ec, span);
  }
  span.Finish();
  if (message_queue_.empty())
    return;
  auto write_event = message_queue_.front();
  message_queue_.pop();
  write_in_progress_ = true;
  do_write(write_event.span, write_event.message);
}
} // namespace ot_chat
