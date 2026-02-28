#include "commands/transport-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── PlayCommand ─────────────────────────────────────────────────────────────

void PlayCommand::execute(AppContext& ctx) { ctx.getAudioEngine().play(); }

// ── PauseCommand ────────────────────────────────────────────────────────────

void PauseCommand::execute(AppContext& ctx) { ctx.getAudioEngine().pause(); }

// ── StopCommand ─────────────────────────────────────────────────────────────

void StopCommand::execute(AppContext& ctx) { ctx.getAudioEngine().stop(); }

// ── SetPlayheadPositionCommand ──────────────────────────────────────────────

SetPlayheadPositionCommand::SetPlayheadPositionCommand(double position)
    : position(position) {}

void SetPlayheadPositionCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().setPlayheadPosition(position);
}

// ── SetCursorPositionCommand ────────────────────────────────────────────────

SetCursorPositionCommand::SetCursorPositionCommand(double position)
    : position(position) {}

void SetCursorPositionCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().setCursorPosition(position);
}

// ── SetMasterVolumeCommand ──────────────────────────────────────────────────

SetMasterVolumeCommand::SetMasterVolumeCommand(float volume)
    : volume(volume) {}

void SetMasterVolumeCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().setMasterVolume(volume);

  broadcast::send(ctx.getWebSocketServer(), "transport.masterVolume",
                  {{"volume", ctx.getAudioEngine().getMasterVolume()}});
}

// Auto-registration
REGISTER_COMMAND("transport.play", PlayCommand);

REGISTER_COMMAND("transport.pause", PauseCommand);

REGISTER_COMMAND("transport.stop", StopCommand);

REGISTER_COMMAND_WITH_CREATOR(
    "transport.setPlayheadPosition", SetPlayheadPosition,
    [](const nlohmann::json& payload) {
      return std::make_unique<SetPlayheadPositionCommand>(
          payload.value("position", 0.0));
    });

REGISTER_COMMAND_WITH_CREATOR(
    "transport.setCursorPosition", SetCursorPosition,
    [](const nlohmann::json& payload) {
      return std::make_unique<SetCursorPositionCommand>(
          payload.value("position", 0.0));
    });

REGISTER_COMMAND_WITH_CREATOR(
    "transport.setMasterVolume", SetMasterVolume,
    [](const nlohmann::json& payload) -> CommandPtr {
      if (!payload.contains("volume")) return nullptr;
      float vol = payload.value("volume", -1.0f);
      if (vol < 0.0f || vol > 1.0f) return nullptr;
      return std::make_unique<SetMasterVolumeCommand>(vol);
    });