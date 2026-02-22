#include "commands/mode-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "commands/command-processor.hpp"
#include "services/mode-manager.hpp"
#include "websocket/websocket-server.hpp"

static constexpr int kEditPollIntervalMs = 1;
static constexpr int kLivePollIntervalMs = 10;
static constexpr int kEditTimerRateMs = 33;   // ~30Hz
static constexpr int kLiveTimerRateMs = 100;  // ~10Hz

void SetEditModeCommand::execute(AppContext& ctx) {
  auto& modeManager = ctx.getModeManager();

  if (!modeManager.setMode(AppMode::Edit)) {
    nlohmann::json error = {{"type", "error"},
                            {"code", "cannot_edit_while_playing"},
                            {"message", "Cannot switch to Edit mode while playing"}};
    reply(error.dump());
    return;
  }

  // Restore edit optimizations: faster polling, higher broadcast rate, unfreeze
  if (auto* proc = ctx.getCommandProcessor()) {
    proc->setPollInterval(kEditPollIntervalMs);
  }
  ctx.getAudioEngine().setTimerRate(kEditTimerRateMs);
  ctx.getAudioEngine().rebuildMonitoredChannelMask();
  ctx.getAudioEngine().unfreezeTracks();

  nlohmann::json broadcast = {{"type", "broadcast"},
                               {"event", "mode.changed"},
                               {"mode", "edit"}};
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

void SetLiveModeCommand::execute(AppContext& ctx) {
  ctx.getModeManager().setMode(AppMode::Live);

  // Live optimizations: slower polling, lower broadcast rate, freeze tracks
  // Monitoring remains functional — the channel mask already avoids unnecessary copies
  if (auto* proc = ctx.getCommandProcessor()) {
    proc->setPollInterval(kLivePollIntervalMs);
  }
  ctx.getAudioEngine().setTimerRate(kLiveTimerRateMs);
  ctx.getAudioEngine().freezeTracks();

  nlohmann::json broadcast = {{"type", "broadcast"},
                               {"event", "mode.changed"},
                               {"mode", "live"}};
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

void GetModeCommand::execute(AppContext& ctx) {
  std::string mode = ctx.getModeManager().isLiveMode() ? "live" : "edit";
  nlohmann::json response = {{"type", "response"},
                              {"event", "mode.current"},
                              {"mode", mode}};
  reply(response.dump());
}

// Auto-registration — accessible in both Live and Edit
REGISTER_COMMAND("mode.setEdit", SetEditModeCommand);
REGISTER_COMMAND("mode.setLive", SetLiveModeCommand);
REGISTER_COMMAND("mode.getMode", GetModeCommand);
