#include "commands/loop-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "services/loop-manager.hpp"
#include "services/project-manager.hpp"
#include "services/songs-manager.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── Helper ────────────────────────────────────────────────────────────────────

static nlohmann::json loopsToJson(const std::vector<Loop>& loops) {
  nlohmann::json arr = nlohmann::json::array();
  for (const auto& l : loops) {
    arr.push_back({{"id", l.id}, {"start", l.start}, {"end", l.end}});
  }
  return arr;
}

static void saveAndBroadcast(AppContext& ctx) {
  Song* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  ctx.getLoopManager().setLoops(song->getLoops());

  ctx.getProjectManager().saveProject(
      ctx.getProjectManager().getCurrentProjectPath(),
      ctx.getSongsManager(), &ctx.getSetlistManager());

  broadcast::send(ctx.getWebSocketServer(), "loop.listUpdated",
                  {{"loops", loopsToJson(song->getLoops())}});
}

// ── LoopAddCommand ────────────────────────────────────────────────────────────

LoopAddCommand::LoopAddCommand(double start, double end)
    : start(start), end(end) {}

void LoopAddCommand::execute(AppContext& ctx) {
  Song* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;
  if (start >= end) return;
  song->addLoop(start, end);
  saveAndBroadcast(ctx);
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "loop.add", LoopAdd,
    [](const nlohmann::json& p) {
      return std::make_unique<LoopAddCommand>(p.value("start", 0.0),
                                             p.value("end", 0.0));
    });

// ── LoopRemoveCommand ─────────────────────────────────────────────────────────

LoopRemoveCommand::LoopRemoveCommand(std::string loopId)
    : loopId(std::move(loopId)) {}

void LoopRemoveCommand::execute(AppContext& ctx) {
  Song* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;
  if (!song->removeLoop(loopId)) return;
  saveAndBroadcast(ctx);
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "loop.remove", LoopRemove,
    [](const nlohmann::json& p) {
      return std::make_unique<LoopRemoveCommand>(p.value("loopId", ""));
    });

// ── LoopUpdateCommand ─────────────────────────────────────────────────────────

LoopUpdateCommand::LoopUpdateCommand(std::string loopId, double start,
                                     double end)
    : loopId(std::move(loopId)), start(start), end(end) {}

void LoopUpdateCommand::execute(AppContext& ctx) {
  Song* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;
  if (start >= end) return;
  if (!song->updateLoop(loopId, start, end)) return;
  ctx.getLoopManager().reset();
  saveAndBroadcast(ctx);
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "loop.update", LoopUpdate,
    [](const nlohmann::json& p) {
      return std::make_unique<LoopUpdateCommand>(p.value("loopId", ""),
                                                 p.value("start", 0.0),
                                                 p.value("end", 0.0));
    });

// ── LoopCancelCommand ─────────────────────────────────────────────────────────

void LoopCancelCommand::execute(AppContext& ctx) {
  if (!ctx.getLoopManager().cancelActiveLoop()) return;
  broadcast::send(ctx.getWebSocketServer(), "loop.deactivated");
}

REGISTER_COMMAND("loop.cancel", LoopCancelCommand);

// ── LoopExitCommand ───────────────────────────────────────────────────────────

void LoopExitCommand::execute(AppContext& ctx) {
  auto endPos = ctx.getLoopManager().exitActiveLoop();
  if (!endPos.has_value()) return;

  ctx.getAudioEngine().setPlayheadPosition(*endPos);
  broadcast::send(ctx.getWebSocketServer(), "loop.deactivated");
  broadcast::send(ctx.getWebSocketServer(), "transport.playheadPosition",
                  {{"position", *endPos}});
}

REGISTER_COMMAND("loop.exit", LoopExitCommand);
