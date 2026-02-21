#include "commands/transport-commands.hpp"

#include <nlohmann/json.hpp>

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

SetPositionCommand::SetPositionCommand(double position)
    : position(std::move(position)) {}

void SetPositionCommand::execute(AppContext& ctx) {
  auto& wsServer = ctx.getWebSocketServer();
  ctx.getAudioEngine().setCurrentPosition(position);
}

// Auto-registration
REGISTER_COMMAND("transport.play", PlayCommand);
REGISTER_COMMAND("transport.pause", PauseCommand);
REGISTER_COMMAND("transport.stop", StopCommand);
REGISTER_COMMAND_WITH_CREATOR("transport.setPosition",
                              [](const nlohmann::json& payload) {
                                return std::make_unique<SetPositionCommand>(
                                    payload.value("position", 0.0));
                              });
