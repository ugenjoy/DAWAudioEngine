#include "commands/song-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── SetTempoCommand ─────────────────────────────────────────────────────────

SetTempoCommand::SetTempoCommand(float tempo) : tempo(tempo) {}

void SetTempoCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  song->setTempo(tempo);

  broadcast::send(ctx.getWebSocketServer(), "song.tempoChanged",
                  {{"tempo", tempo}});
}

// ── SetMetronomeMuteCommand ──────────────────────────────────────────────────

SetMetronomeMuteCommand::SetMetronomeMuteCommand(bool mute) : mute(mute) {}

void SetMetronomeMuteCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* metronome = song->getMetronomeTrack();
  if (!metronome) return;

  metronome->setMute(mute);

  broadcast::send(ctx.getWebSocketServer(), "song.metronomeMuteChanged",
                  {{"mute", mute}});
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
