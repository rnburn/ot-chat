#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include "configuration.h"
#include "chat_room.h"
using tcp = boost::asio::ip::tcp;


namespace ot_chat {
class chat_server {
 public:
  chat_server(const configuration& config, boost::asio::io_context& ioc);

  void run();

 private:
  const configuration& config_;
  tcp::acceptor acceptor_;
  tcp::socket socket_;
  chat_room chat_room_;

  void do_accept();

  void on_accept(boost::system::error_code ec);
};
} // namespace ot_chat
