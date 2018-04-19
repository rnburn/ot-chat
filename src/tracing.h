#pragma once

#include <opentracing/span.h>
#include <opentracing/ext/tags.h>
#include <boost/system/error_code.hpp>

namespace ot_chat {
void set_span_error_code(boost::system::error_code ec,
    opentracing::Span& span);
} // namespace ot_chat
