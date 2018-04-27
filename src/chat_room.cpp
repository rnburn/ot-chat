#include "chat_room.h"
#include "chat_session.h"
#include <mutex>
#include <algorithm>
#include <cassert>

namespace ot_chat {
chat_room::chat_room(const configuration& config) : config_{config} {}

/* void chat_room::broadcast( */
/*     const opentracing::SpanContext* prev_span_context, */
/*     const std::shared_ptr<const boost::beast::multi_buffer>& message) { */
/*   auto span = config_.tracer->StartSpan( */
/*       "BroadcastMsg", {opentracing::FollowsFrom(prev_span_context)}); */
/*   std::lock_guard<std::mutex> lock_guard{mutex_}; */
/*   for (auto& session : sessions_) */
/*     session->send(&span->context(), message); */
/* } */

void chat_room::broadcast(
      const opentracing::SpanContext* prev_span_context,
      const std::shared_ptr<const std::string>& text) {
  auto span = config_.tracer->StartSpan(
      "BroadcastMsg", {opentracing::FollowsFrom(prev_span_context)});
  std::lock_guard<std::mutex> lock_guard{mutex_};
  for (auto& session : sessions_)
    session->send(&span->context(), text);
}

void chat_room::enter(chat_session* session) {
  std::lock_guard<std::mutex> lock_guard{mutex_};
  sessions_.push_back(session);
}

void chat_room::leave(const chat_session* session) {
  std::lock_guard<std::mutex> lock_guard{mutex_};
  auto iter = std::find(std::begin(sessions_), std::end(sessions_), session);
  assert(iter != std::end(sessions_));
  sessions_.erase(iter);
}
} // namespace ot_chat
