#include "commands/song-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "websocket/websocket-server.hpp"

SetTempoCommand::SetTempoCommand(float tempo) : tempo(tempo) {}

void SetTempoCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  song->setTempo(tempo);

  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "song.tempoChanged";
  broadcast["tempo"] = tempo;
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

SetMetronomeMuteCommand::SetMetronomeMuteCommand(bool mute) : mute(mute) {}

void SetMetronomeMuteCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* metronome = song->getMetronomeTrack();
  if (!metronome) return;

  metronome->setMute(mute);

  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "song.metronomeMuteChanged";
  broadcast["mute"] = mute;
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

// Auto-registration
REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "song.setTempo", SetTempo,
    [](const nlohmann::json& payload) -> CommandPtr {
      float tempo = payload.value("tempo", 0.0f);
      if (tempo < 20.0f || tempo > 999.0f) return nullptr;
      return std::make_unique<SetTempoCommand>(tempo);
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "song.setMetronomeMute", SetMetronomeMute,
    [](const nlohmann::json& payload) -> CommandPtr {
      bool mute = payload.value("mute", true);
      return std::make_unique<SetMetronomeMuteCommand>(mute);
    });
