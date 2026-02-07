#include "commands/transport-commands.hpp"
#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"

void PlayCommand::execute(AppContext& ctx) { ctx.getAudioEngine().play(); }

void PauseCommand::execute(AppContext& ctx) { ctx.getAudioEngine().pause(); }

void StopCommand::execute(AppContext& ctx) { ctx.getAudioEngine().stop(); }

// Auto-registration
REGISTER_COMMAND("transport.play", PlayCommand);
REGISTER_COMMAND("transport.pause", PauseCommand);
REGISTER_COMMAND("transport.stop", StopCommand);
