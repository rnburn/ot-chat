#include <gflags/gflags.h>
#include <opentracing/dynamic_load.h>
#include <opentracing/noop.h>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <thread>
#include <vector>
#include "chat_server.h"
#include "configuration.h"
using namespace ot_chat;

DEFINE_string(config, "", "Configuration file for ot-chat");

int main(int argc, char* argv[]) {
  auto logger = spdlog::stdout_color_mt("console");
  try {
    // Parse and Validate flags
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_config.empty()) {
      std::cout << "Usage: ot-chat --config=<config_file>\n";
      return 1;
    }

    const auto config = load_configuration(logger, FLAGS_config.c_str());

    auto const threads = 4;

    // Set up the chat server
    boost::asio::io_context ioc{threads};
    chat_server server{config, ioc};
    server.run();

    // Start ASIO threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
      v.emplace_back([&ioc] { ioc.run(); });
    ioc.run();

    for (auto& thread : v) thread.join();

    config.tracer->Close();

    return 0;
  } catch (const std::exception& e) {
    logger->error(e.what());
    return -1;
  }
}
