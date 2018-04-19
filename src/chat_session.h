#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include "chat_room.h"
#include "configuration.h"
using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace websocket = boost::beast::websocket;

namespace ot_chat {
class chat_session : public std::enable_shared_from_this<chat_session> {
 public:
  chat_session(const configuration& config, chat_room& chat_room,
               tcp::socket&& socket);

  void run(const opentracing::SpanContext& read_context,
           http::request<http::string_body>&& req);

  void send(const opentracing::SpanContext* parent_span_context,
            const std::shared_ptr<const boost::beast::multi_buffer>& message);

 private:
  const configuration& config_;
  chat_room& chat_room_;
  websocket::stream<tcp::socket> ws_;

  std::mutex write_mutex_;
  bool write_in_progress_{false};
  struct WriteEvent {
    std::shared_ptr<opentracing::Span> span;
    std::shared_ptr<const boost::beast::multi_buffer> message;
  };
  std::queue<WriteEvent> message_queue_;

  void do_read();
  void do_write(
      const std::shared_ptr<opentracing::Span>& span,
      const std::shared_ptr<const boost::beast::multi_buffer>& buffer);
  void on_accept(opentracing::Span& span, boost::system::error_code ec);
  void on_read(boost::system::error_code ec,
               const std::shared_ptr<boost::beast::multi_buffer>& buffer);
  void on_write(opentracing::Span& span, boost::system::error_code ec);
};
} // namespace ot_chat
