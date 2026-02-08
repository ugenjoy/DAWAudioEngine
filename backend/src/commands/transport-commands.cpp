#include "commands/transport-commands.hpp"
#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "websocket/websocket-server.hpp"

void PlayCommand::execute(AppContext& ctx) {
  auto& wsServer = ctx.getWebSocketServer();

  ctx.getAudioEngine().play();

  // Broadcast transport play event to all clients
  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "transport.play";

  wsServer.broadcast(broadcast.dump());
}

void PauseCommand::execute(AppContext& ctx) {
  auto& wsServer = ctx.getWebSocketServer();

  ctx.getAudioEngine().pause();

  // Broadcast transport play event to all clients
  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "transport.pause";

  wsServer.broadcast(broadcast.dump());
}

void StopCommand::execute(AppContext& ctx) {
  auto& wsServer = ctx.getWebSocketServer();

  ctx.getAudioEngine().stop();

  // Broadcast transport play event to all clients
  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "transport.stop";

  wsServer.broadcast(broadcast.dump());
}

// Auto-registration
REGISTER_COMMAND("transport.play", PlayCommand);
REGISTER_COMMAND("transport.pause", PauseCommand);
REGISTER_COMMAND("transport.stop", StopCommand);
