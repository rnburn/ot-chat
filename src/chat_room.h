#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <boost/beast/core/multi_buffer.hpp>
#include "configuration.h"

namespace ot_chat {
class chat_session;

class chat_room {
 public:
  explicit chat_room(const configuration& config);

  void broadcast(
      const opentracing::SpanContext* prev_span_context,
      const std::shared_ptr<const boost::beast::multi_buffer>& message);

  void enter(chat_session* chat_session);
  void leave(const chat_session* chat_session);
 private:
  const configuration& config_;
  std::mutex mutex_;
   std::vector<chat_session*> sessions_;
};
} // namespace ot_chat
