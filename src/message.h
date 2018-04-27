#pragma once

#include "configuration.h"


#include <string>

namespace ot_chat {
bool deserialize(const configuration& config,
                 const std::string& json,
                 std::unique_ptr<opentracing::SpanContext>& span_context,
                 std::string& text);

void serialize(const configuration& config,
               const opentracing::SpanContext& span_context, const std::string& text,
               std::string& json);
} // namespace ot_chat
