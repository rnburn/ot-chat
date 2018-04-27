#include "message.h"
#include "chat_message.pb.h"

#include <google/protobuf/util/json_util.h>

#include <cassert>

namespace ot_chat {
namespace {
class carrier_reader : public opentracing::TextMapReader {
 public:
  explicit carrier_reader(
      const google::protobuf::Map<std::string, std::string>& span_context_map)
      : span_context_map_{span_context_map} {}
  using F = std::function<opentracing::expected<void>(
      opentracing::string_view, opentracing::string_view)>;

  opentracing::expected<void> ForeachKey(F f) const override {
    for (auto& entry : span_context_map_) {
      auto was_successful = f(entry.first, entry.second);
      if (!was_successful) return was_successful;
    }
    return {};
  }

 private:
  const google::protobuf::Map<std::string, std::string>& span_context_map_;
};
}  // namespace

bool deserialize(const configuration& config, const std::string& json,
                 std::unique_ptr<opentracing::SpanContext>& span_context,
                 std::string& text) {
  ChatMessage message;
  auto parse_result =
      google::protobuf::util::JsonStringToMessage(json, &message);
  if (!parse_result.ok()) return false;

  text = std::move(*message.mutable_content());

  carrier_reader carrier{message.span_context()};
  auto span_context_maybe = config.tracer->Extract(carrier);
  if (span_context_maybe)
    span_context = std::move(*span_context_maybe);
  else
    config.logger->error("failed to extract opentracing::SpanContext: {}",
                         span_context_maybe.error().message());

  return true;
}

namespace {
class carrier_writer : public opentracing::TextMapWriter {
 public:
  explicit carrier_writer(
      google::protobuf::Map<std::string, std::string>& span_context_map)
      : span_context_map_{span_context_map} {}

  opentracing::expected<void> Set(
      opentracing::string_view key,
      opentracing::string_view value) const override {
    google::protobuf::Map<std::string, std::string>::value_type element{key,
                                                                        value};
    span_context_map_.insert(std::move(element));
    return {};
  }

 private:
  google::protobuf::Map<std::string, std::string>& span_context_map_;
};
}  // namespace

void serialize(const configuration& config,
               const opentracing::SpanContext& span_context,
               const std::string& text, std::string& json) {
  ChatMessage message;
  message.set_content(text);
  carrier_writer carrier{*message.mutable_span_context()};
  auto was_inserted = config.tracer->Inject(span_context, carrier);
  if (!was_inserted)
    config.logger->error("failed to inject opentracing::SpanContext: {}",
                         was_inserted.error().message());
  auto convert_result =
      google::protobuf::util::MessageToJsonString(message, &json);
  if (!convert_result.ok()) {
    config.logger->critical("failed to serialize ChatMessage: {}",
                            convert_result.ToString());
    std::terminate();
  }
}
}  // namespace ot_chat
