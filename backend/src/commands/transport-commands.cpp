#include "commands/transport-commands.hpp"
#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "websocket/websocket-server.hpp"

void PlayCommand::execute(AppContext& ctx) {
  auto& wsServer = ctx.getWebSocketServer();
  ctx.getAudioEngine().play();
}

void PauseCommand::execute(AppContext& ctx) {
  auto& wsServer = ctx.getWebSocketServer();
  ctx.getAudioEngine().pause();
}

void StopCommand::execute(AppContext& ctx) {
  auto& wsServer = ctx.getWebSocketServer();
  ctx.getAudioEngine().stop();
}

// Auto-registration
REGISTER_COMMAND("transport.play", PlayCommand);
REGISTER_COMMAND("transport.pause", PauseCommand);
REGISTER_COMMAND("transport.stop", StopCommand);
