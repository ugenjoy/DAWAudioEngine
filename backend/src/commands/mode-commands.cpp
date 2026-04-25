#include "commands/mode-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "commands/command-processor.hpp"
#include "services/mode-manager.hpp"
#include "services/project-manager.hpp"
#include "services/song-preloader.hpp"
#include "services/songs-manager.hpp"
#include "websocket/websocket-server.hpp"

static constexpr int kEditPollIntervalMs = 1;
static constexpr int kLivePollIntervalMs = 10;

// ── SetEditModeCommand ──────────────────────────────────────────────────

void SetEditModeCommand::execute(AppContext& ctx) {
  auto& modeManager = ctx.getModeManager();

  if (!modeManager.setMode(AppMode::Edit)) {
    nlohmann::json error = {{"type", "error"},
                            {"code", "cannot_edit_while_playing"},
                            {"message", "Cannot switch to Edit mode while playing"}};
    reply(error.dump());
    return;
  }

  // Restore edit optimizations: faster polling, unfreeze
  if (auto* proc = ctx.getCommandProcessor()) {
    proc->setPollInterval(kEditPollIntervalMs);
  }
  ctx.getAudioEngine().rebuildMonitoredChannelMask();
  ctx.getAudioEngine().unfreezeTracks();

  // Cancel any pending preload
  ctx.getSongPreloader().onLiveModeExited();
  ctx.getLiveSetlistManager().unload(ctx);

  nlohmann::json broadcast = {{"type", "broadcast"},
                              {"event", "mode.changed"},
                              {"mode", "edit"}};
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

REGISTER_COMMAND("mode.setEdit", SetEditModeCommand);

// ── SetLiveModeCommand ──────────────────────────────────────────────────

void SetLiveModeCommand::execute(AppContext& ctx) {
  ctx.getModeManager().setMode(AppMode::Live);

  // Live optimizations: slower polling, freeze tracks
  // Monitoring remains functional — the channel mask already avoids unnecessary copies
  if (auto* proc = ctx.getCommandProcessor()) {
    proc->setPollInterval(kLivePollIntervalMs);
  }
  ctx.getAudioEngine().freezeTracks();

  // Preload next song
  auto* activeSong = ctx.getAudioEngine().getActiveSong();
  if (activeSong) {
    auto songs = ctx.getSongsManager().getSongList();
    ctx.getSongPreloader().onLiveModeEntered(
        activeSong, songs, ctx.getProjectManager().getAudioDir());
  }

  nlohmann::json broadcast = {{"type", "broadcast"},
                              {"event", "mode.changed"},
                              {"mode", "live"}};
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

REGISTER_COMMAND("mode.setLive", SetLiveModeCommand);

// ── GetModeCommand ──────────────────────────────────────────────────

void GetModeCommand::execute(AppContext& ctx) {
  std::string mode = ctx.getModeManager().isLiveMode() ? "live" : "edit";
  nlohmann::json response = {{"type", "response"},
                             {"event", "mode.current"},
                             {"mode", mode}};
  reply(response.dump());
}

REGISTER_COMMAND("mode.getMode", GetModeCommand);
