#pragma once

#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "configuration.h"
#include "chat_room.h"
using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

namespace ot_chat {
class http_session : public std::enable_shared_from_this<http_session> {
 public:
  http_session(const configuration& config, chat_room& chat_room,
               tcp::socket&& socket);

  void run();
 private:
  const configuration& config_;
  chat_room& chat_room_;
  tcp::socket socket_;
  http::request<http::string_body> req_;
  http::response<http::span_body<const char>> res_;
  boost::beast::flat_buffer buffer_;

  void do_read();
  void on_read(boost::system::error_code ec, std::size_t bytes_transferred);

  void do_close();

  void on_write(opentracing::Span& write_span, boost::system::error_code ec,
                std::size_t bytes_transferred, bool close);
};
} // namespace ot_chat
