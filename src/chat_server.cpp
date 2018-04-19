#include "chat_server.h"
#include "http_session.h"

namespace ot_chat {
chat_server::chat_server(const configuration& config,
                         boost::asio::io_context& ioc)
    : config_{config}, acceptor_{ioc}, socket_{ioc}, chat_room_{config} {
  boost::system::error_code ec;

  // Open the acceptor
  acceptor_.open(config.endpoint.protocol(), ec);
  if (ec) {
    config_.logger->error("open failed: {}", ec.message());
    return;
  }

  // Bind to the server address
  acceptor_.bind(config.endpoint, ec);
  if (ec) {
    config_.logger->error("bind failed: {}", ec.message());
    return;
  }

  // Start listening for connections
  acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    config_.logger->error("listen failed: {}", ec.message());
    return;
  }
}

void chat_server::run() {
  if (!acceptor_.is_open()) return;
  do_accept();
}

void chat_server::do_accept() {
  acceptor_.async_accept(
      socket_, [this](boost::system::error_code ec) {
        on_accept(ec);
      });
}

void chat_server::on_accept(boost::system::error_code ec) {
  if (ec) {
    config_.logger->info("accept failed: {}", ec.message());
  } else {
    // Create the http_session and run it
    std::make_shared<http_session>(config_, chat_room_, std::move(socket_))->run();
  }

  // Accept another connection
  do_accept();
}
} // namespace ot_chat
