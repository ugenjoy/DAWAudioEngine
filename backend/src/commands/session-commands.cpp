#include "commands/session-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "events/event-engine.hpp"
#include "model/project.hpp"
#include "model/song.hpp"
#include "services/live-setlist-manager.hpp"
#include "services/session.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── OpenProjectCommand ───────────────────────────────────────────────────────

OpenProjectCommand::OpenProjectCommand(std::string path)
    : path(std::move(path)) {}

void OpenProjectCommand::execute(AppContext& ctx) {
  auto project = Project::loadFromDisk(path);
  if (!project) {
    broadcast::send(ctx.getWebSocketServer(), "session.openFailed",
                    {{"path", path}, {"error", "Failed to load project"}});
    return;
  }

  ctx.getAudioEngine().stop();
  ctx.getAudioEngine().unloadSong();
  ctx.getLoopManager().reset();
  ctx.getEventEngine().loadRules({}, {}, {});

  // Cache songs JSON before moving the project into session
  nlohmann::json songsJson = project->songs().toJson();

  ctx.getSession().openProject(std::move(project));

  // Broadcast for frontend compatibility (session.changed is fired by listener)
  broadcast::send(ctx.getWebSocketServer(), "project.songsUpdated",
                  {{"songs", songsJson}});
}

REGISTER_COMMAND_WITH_CREATOR("session.openProject", SessionOpenProject,
                              [](const nlohmann::json& payload) -> CommandPtr {
                                std::string path = payload.value("path", "");
                                if (path.empty()) return nullptr;
                                return std::make_unique<OpenProjectCommand>(
                                    path);
                              });

// ── CloseProjectCommand ──────────────────────────────────────────────────────

void CloseProjectCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().stop();
  ctx.getAudioEngine().unloadSong();
  ctx.getLoopManager().reset();
  ctx.getEventEngine().loadRules({}, {}, {});
  ctx.getSession().closeProject();
  // session.changed broadcast comes from listener
}

REGISTER_COMMAND("session.closeProject", CloseProjectCommand);

// ── StartEditingCommand ──────────────────────────────────────────────────────

StartEditingCommand::StartEditingCommand(std::string songId)
    : songId(std::move(songId)) {}

void StartEditingCommand::execute(AppContext& ctx) {
  auto& session = ctx.getSession();
  auto* project = session.getProject();
  if (!project) return;

  Song* song = project->songs().getSongById(songId);
  if (!song) return;

  // Stop current audio
  ctx.getAudioEngine().stop();
  auto* activeSong = ctx.getAudioEngine().getActiveSong();
  if (activeSong && activeSong != song) {
    activeSong->unloadAudio();
    broadcast::send(ctx.getWebSocketServer(), "song.unloaded",
                    {{"songId", activeSong->getId()}});
  }

  // Load song audio if needed
  const std::string audioDir = project->getAudioDir();
  if (song->getLoadState() != SongLoadState::Loaded) {
    broadcast::send(ctx.getWebSocketServer(), "song.loading",
                    {{"songId", song->getId()}});
    song->loadAudio(audioDir);
  }

  ctx.getAudioEngine().loadSong(song);
  ctx.getLoopManager().reset();
  ctx.getLoopManager().setLoops(song->getLoops());
  ctx.getEventEngine().loadRules(project->getEventRules(), {},
                                 song->getEventRules());
  ctx.getEventEngine().loadMarkers(song->getMarkers());
  ctx.getEventEngine().fire("song.loaded", ctx);

  // Update session state — triggers session.changed broadcast via listener
  session.startEditing(songId);

  broadcast::send(ctx.getWebSocketServer(), "song.loaded",
                  {{"song", song->toJson()}});
}

REGISTER_COMMAND_WITH_CREATOR("session.startEditing", SessionStartEditing,
                              [](const nlohmann::json& payload) -> CommandPtr {
                                std::string id = payload.value("songId", "");
                                if (id.empty()) return nullptr;
                                return std::make_unique<StartEditingCommand>(
                                    id);
                              });

// ── StartPerformingCommand ───────────────────────────────────────────────────

StartPerformingCommand::StartPerformingCommand(std::string setlistId)
    : setlistId(std::move(setlistId)) {}

void StartPerformingCommand::execute(AppContext& ctx) {
  auto& session = ctx.getSession();
  auto* project = session.getProject();
  if (!project) return;

  auto* setlist = project->setlists().findById(setlistId);
  if (!setlist || setlist->getEntries().empty()) return;

  std::vector<Song*> songs;
  for (const auto& e : setlist->getEntries()) {
    if (auto* s = project->songs().getSongById(e.songId))
      songs.push_back(s);
  }

  // Delegate to LiveSetlistManager — it handles audio loading, broadcasting,
  // and calls session.startPerforming() via its Session shim.
  ctx.getLiveSetlistManager().load(*setlist, songs, ctx);
}

REGISTER_COMMAND_WITH_CREATOR(
    "session.startPerforming", SessionStartPerforming,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string id = payload.value("setlistId", "");
      if (id.empty()) return nullptr;
      return std::make_unique<StartPerformingCommand>(id);
    });

// ── StopActivityCommand ──────────────────────────────────────────────────────

void StopActivityCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().stop();
  ctx.getAudioEngine().unloadSong();
  ctx.getLoopManager().reset();
  ctx.getEventEngine().loadRules({}, {}, {});
  if (ctx.getSession().isLive()) {
    // LiveSetlistManager::unload() calls session.stopActivity() via shim
    ctx.getLiveSetlistManager().unload(ctx);
  } else {
    ctx.getSession().stopActivity();
  }
}

REGISTER_COMMAND("session.stopActivity", StopActivityCommand);

// ── SessionAdvanceCommand ────────────────────────────────────────────────────

void SessionAdvanceCommand::execute(AppContext& ctx) {
  ctx.getLiveSetlistManager().advance(ctx);
}

REGISTER_COMMAND("session.advance", SessionAdvanceCommand);

// ── SessionPreviousCommand ───────────────────────────────────────────────────

void SessionPreviousCommand::execute(AppContext& ctx) {
  ctx.getLiveSetlistManager().previous(ctx);
}

REGISTER_COMMAND("session.previous", SessionPreviousCommand);

// ── SessionGoToCommand ───────────────────────────────────────────────────────

SessionGoToCommand::SessionGoToCommand(int index) : index(index) {}

void SessionGoToCommand::execute(AppContext& ctx) {
  ctx.getLiveSetlistManager().goTo(index, ctx);
}

REGISTER_COMMAND_WITH_CREATOR(
    "session.goTo", SessionGoTo,
    [](const nlohmann::json& payload) -> CommandPtr {
      if (!payload.contains("index")) return nullptr;
      int index = payload.value("index", 0);
      return std::make_unique<SessionGoToCommand>(index);
    });

// ── GetSessionCommand ────────────────────────────────────────────────────────

void GetSessionCommand::execute(AppContext& ctx) {
  auto& session = ctx.getSession();
  auto& audioEngine = ctx.getAudioEngine();

  nlohmann::json response;
  response["type"] = "response";
  response["event"] = "session.currentState";

  nlohmann::json payload = session.toJson();
  payload["playback"] = {
      {"isPlaying", audioEngine.isPlaying()},
      {"playheadPosition", audioEngine.getPlayheadPosition()},
      {"cursorPosition", audioEngine.getCursorPosition()},
      {"masterVolume", audioEngine.getMasterVolume()}};

  // Backward compatibility: expose live setlist state at the top level
  if (session.isLive()) {
    auto& lsm = ctx.getLiveSetlistManager();
    payload["setlist"] = lsm.getSetlist().toJson();
    payload["currentIndex"] = lsm.getCurrentIndex();
  }

  response.merge_patch(payload);
  reply(response.dump());
}

REGISTER_COMMAND("session.get", GetSessionCommand);
