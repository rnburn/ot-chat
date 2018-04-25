#include "configuration.h"
#include "configuration.pb.h"

#include <google/protobuf/util/json_util.h>
#include <opentracing/noop.h>
#include <boost/filesystem.hpp>

#include <fstream>
#include <stdexcept>
#include <cerrno>
#include <limits>
#include <cstddef>

namespace ot_chat {
static std::string read_file(const char* filename) {
  errno = 0;
  std::ifstream in{filename};
  if (!in.good()) {
    throw std::runtime_error{
        fmt::format("Failed to open {} {}", filename, std::strerror(errno))};
  }
  in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  return std::string{std::istreambuf_iterator<char>{in},
                     std::istreambuf_iterator<char>{}};
}

static tcp::endpoint make_endpoint(const Configuration& config) {
  boost::system::error_code error_code;
  auto const address = boost::asio::ip::make_address(config.address(), error_code);
  if (error_code)
    throw std::invalid_argument{
        fmt::format("Invalid address {}", config.address())};

  if (config.port() == 0 ||
      config.port() > std::numeric_limits<std::uint16_t>::max())
    throw std::invalid_argument{fmt::format("Invalid port {}", config.port())};

  return {address, static_cast<std::uint16_t>(config.port())};
}

static void load_tracer(const Configuration& config_protobuf,
                        configuration& config) {
  std::string error_message;

  // Dynamically load the vendor's tracing library
  auto handle_maybe = opentracing::DynamicallyLoadTracingLibrary(
      config_protobuf.tracer_library().c_str(), error_message);

  // Check to see if we loaded the tracing library successfully; fail with a
  // readable message if not.
  if (!handle_maybe) throw std::runtime_error{error_message};

  config.tracing_library_handle = std::move(*handle_maybe);

  // Serialize the tracer's configuration to JSON and attempt to construct a
  // tracer from it. Since the configuration is vendor-specific, validating and
  // providing a useful error message is the responsibility of the vendor's
  // library.
  std::string tracer_config_json;
  auto convert_result = google::protobuf::util::MessageToJsonString(
      config_protobuf.tracer_configuration(), &tracer_config_json);
  if (!convert_result.ok())
    throw std::runtime_error{
        fmt::format("Failed to serialize tracer_config to JSON {}",
                    convert_result.ToString().c_str())};

  auto tracer_maybe = config.tracing_library_handle.tracer_factory().MakeTracer(
      tracer_config_json.data(), error_message);

  // If we failed to construct the vendor's tracer (e.g. configuration was
  // invalid), then error_message should contain a useful message.
  if (!tracer_maybe) throw std::runtime_error{error_message};

  config.tracer = std::move(*tracer_maybe);
}

static void load_resources_impl(
    const boost::filesystem::path& html_root,
    const boost::filesystem::path& directory,
    std::unordered_map<std::string, resource>& resources) {
  for (auto& file_path : boost::filesystem::directory_iterator(directory)) {
    if (boost::filesystem::is_directory(file_path))
      return load_resources_impl(html_root, file_path, resources);
    if (!boost::filesystem::is_regular_file(file_path)) continue;

    ot_chat::resource resource;
    auto path = file_path.path();
    if (path.extension() == ".html")
      resource.content_type = "text/html";
    else if (path.extension() == ".js")
      resource.content_type = "application/javascript";
    else
      continue;
    resource.content = read_file(path.string().c_str());
    resources["/" + boost::filesystem::relative(path, html_root).string()] =
        std::move(resource);
  }
}

static std::unordered_map<std::string, resource> load_resources(
    const boost::filesystem::path& html_root) {
  if (!boost::filesystem::is_directory(html_root))
    throw std::runtime_error{
        fmt::format("{} must be a directory", html_root.string())};
  std::unordered_map<std::string, resource> result;
  load_resources_impl(html_root, html_root, result);
  return result;
}

configuration load_configuration(const std::shared_ptr<spdlog::logger>& logger,
                                 const char* config_file) {
  auto config_json = read_file(config_file);

  // Convert the configuration provided as JSON to the configuration protobuf 
  // representation.
  Configuration config_protobuf;
  auto parse_result = google::protobuf::util::JsonStringToMessage(
      config_json, &config_protobuf);
  if (!parse_result.ok()) 
    throw std::invalid_argument{fmt::format("Invalid configuration {}",
                                            parse_result.ToString().c_str())};
  configuration result;
  result.logger = logger;

  result.endpoint = make_endpoint(config_protobuf);

  if (config_protobuf.tracer_library().empty())
    result.tracer = opentracing::MakeNoopTracer();
  else
    load_tracer(config_protobuf, result);
  // opentracing::MakeNoopTracer returns a nullptr on failure, so check for this
  // condition.
  if (result.tracer == nullptr)
    throw std::runtime_error("Failed to construct a tracer");

  result.resources = load_resources(config_protobuf.html_directory());

  return result;
}
} // namespace ot_chat
