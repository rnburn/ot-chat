#include "tracing.h"

namespace ot_chat {
void set_span_error_code(boost::system::error_code ec,
    opentracing::Span& span) {
    span.SetTag(opentracing::ext::error, true);
    span.Log({
        {"event", "error"},
        {"message", ec.message()}
    });
}
} // namespace ot_chat
