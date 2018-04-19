#pragma once

#include <boost/utility/string_view.hpp>
#include <spdlog/spdlog.h>
#include <opentracing/tracer.h>
#include <boost/asio/ip/tcp.hpp>
#include <opentracing/dynamic_load.h>
using tcp = boost::asio::ip::tcp;

namespace ot_chat {
struct configuration {
  std::shared_ptr<spdlog::logger> logger;


  // If a tracer is dynamically loaded, tracer_library_handle represents
  // the resource obtained via dlopen. This needs to outlive tracer, so
  // the declaration order is important.
  opentracing::DynamicTracingLibraryHandle tracing_library_handle;


  // If specified, tracer is the dynamically loaded vendor tracer; otherwise,
  // it's set to the noop tracer.
  std::shared_ptr<opentracing::Tracer> tracer;

  tcp::endpoint endpoint;
  std::string html;
};

configuration load_configuration(const std::shared_ptr<spdlog::logger>& logger,
                                 const char* config_file);
}  // namespace ot_chat
