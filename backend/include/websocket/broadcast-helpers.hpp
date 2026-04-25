#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "websocket/websocket-server.hpp"

namespace broadcast {

/**
 * Build a broadcast envelope and send it to all connected clients.
 * Automatically wraps data in {"type": "broadcast", "event": <event>, ...data}.
 */
inline void send(WebSocketServer& ws, const std::string& event,
                 const nlohmann::json& data = {}) {
  nlohmann::json msg = data;
  msg["type"] = "broadcast";
  msg["event"] = event;
  ws.broadcast(msg.dump());
}

}  // namespace broadcast
