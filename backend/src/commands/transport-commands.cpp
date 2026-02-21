#include "commands/transport-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"

void PlayCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().play();
}

void PauseCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().pause();
}

void StopCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().stop();
}

SetPositionCommand::SetPositionCommand(double position) : position(position) {}

void SetPositionCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().setCurrentPosition(position);
}

// Auto-registration
REGISTER_COMMAND("transport.play", PlayCommand);
REGISTER_COMMAND("transport.pause", PauseCommand);
REGISTER_COMMAND("transport.stop", StopCommand);
REGISTER_COMMAND_WITH_CREATOR("transport.setPosition", SetPosition,
                              [](const nlohmann::json& payload) {
                                return std::make_unique<SetPositionCommand>(
                                    payload.value("position", 0.0));
                              });
