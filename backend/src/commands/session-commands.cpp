#include "commands/session-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "events/event-engine.hpp"
#include "model/project.hpp"
#include "model/song.hpp"
#include "services/session.hpp"
#include "services/song-preloader.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── Helper: load a song into the audio engine for live performance ────────────

static void loadSongForPerforming(Song* song, Project* project, AppContext& ctx,
                                  bool fireEvents) {
  if (!song || !project) return;

  const std::string audioDir = project->getAudioDir();

  if (song->getLoadState() != SongLoadState::Loaded) {
    song->loadAudio(audioDir);
  }

  ctx.getAudioEngine().loadSong(song);
  ctx.getLoopManager().reset();
  ctx.getLoopManager().setLoops(song->getLoops());

  const Setlist* setlist = nullptr;
  if (auto id = ctx.getSession().getPerformingSetlistId()) {
    setlist = project->setlists().findById(*id);
  }

  ctx.getEventEngine().loadRules(
      project->songs().getProjectEventRules(),
      setlist ? setlist->getEventRules() : std::vector<EventRule>{},
      song->getEventRules());
  ctx.getEventEngine().loadMarkers(song->getMarkers());

  if (fireEvents) {
    ctx.getEventEngine().fire("song.loaded", ctx);
  }
}

static void preloadNextSong(Project* project, AppContext& ctx) {
  if (!project) return;
  auto id = ctx.getSession().getPerformingSetlistId();
  if (!id) return;

  const Setlist* setlist = project->setlists().findById(*id);
  if (!setlist) return;

  const auto& entries = setlist->getEntries();
  int idx = ctx.getSession().getPerformingIndex();

  // Build ordered song list for preloader
  std::vector<Song*> orderedSongs;
  for (const auto& e : entries) {
    if (auto* s = project->songs().getSongById(e.songId))
      orderedSongs.push_back(s);
  }

  Song* currentSong = nullptr;
  if (idx >= 0 && idx < static_cast<int>(orderedSongs.size()))
    currentSong = orderedSongs[idx];

  if (currentSong) {
    ctx.getSongPreloader().onSongChanged(currentSong, orderedSongs,
                                         project->getAudioDir());
  }
}

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

  // Freeze tracks for live performance
  ctx.getAudioEngine().stop();
  ctx.getAudioEngine().freezeTracks();
  ctx.getSongPreloader().onLiveModeExited();  // reset any edit preloads

  // Update session state first so getPerformingSetlistId() works in helpers
  session.startPerforming(setlistId);

  // Load first song
  const auto& entries = setlist->getEntries();
  Song* firstSong = project->songs().getSongById(entries[0].songId);
  if (firstSong) {
    loadSongForPerforming(firstSong, project, ctx, false);
    ctx.getEventEngine().fire("setlist.started", ctx);

    // Preload next song
    preloadNextSong(project, ctx);

    broadcast::send(ctx.getWebSocketServer(), "setlist.loaded",
                    {{"setlist", setlist->toJson()},
                     {"currentIndex", 0},
                     {"song", firstSong->toJson()}});
  }
  // session.changed is broadcast by listener
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
  bool wasLive = ctx.getSession().isLive();
  ctx.getAudioEngine().stop();
  ctx.getAudioEngine().unloadSong();
  ctx.getLoopManager().reset();
  ctx.getEventEngine().loadRules({}, {}, {});
  if (wasLive) {
    ctx.getAudioEngine().unfreezeTracks();
    ctx.getSongPreloader().onLiveModeExited();
    broadcast::send(ctx.getWebSocketServer(), "setlist.unloaded", {});
  }
  ctx.getSession().stopActivity();
  // session.changed broadcast comes from listener
}

REGISTER_COMMAND("session.stopActivity", StopActivityCommand);

// ── SessionAdvanceCommand ────────────────────────────────────────────────────

SessionAdvanceCommand::SessionAdvanceCommand(bool keepPlaying)
    : keepPlaying(keepPlaying) {}

void SessionAdvanceCommand::execute(AppContext& ctx) {
  auto& session = ctx.getSession();
  auto* project = session.getProject();
  if (!project || !session.isLive()) return;

  auto setlistId = session.getPerformingSetlistId();
  if (!setlistId) return;

  const Setlist* setlist = project->setlists().findById(*setlistId);
  if (!setlist) return;

  int currentIdx = session.getPerformingIndex();
  int nextIdx = currentIdx + 1;
  if (nextIdx >= static_cast<int>(setlist->getEntries().size())) return;

  const auto& entries = setlist->getEntries();
  Song* oldSong = project->songs().getSongById(entries[currentIdx].songId);

  // Advance session index — fires listener (session.changed)
  if (!session.advancePerforming()) return;

  Song* newSong = project->songs().getSongById(entries[nextIdx].songId);
  if (!newSong) return;

  // Manual advance: stop transport. Continue transition: skip the stop so
  // playback stays "playing" through the swap; loadSong() resets the
  // end-position clamp and the audio thread resumes on the new song.
  if (!keepPlaying) {
    ctx.getAudioEngine().stop();
  }

  // Unload old song if not preloaded
  if (oldSong && oldSong != newSong &&
      oldSong != ctx.getSongPreloader().getPreloadedSong() &&
      oldSong->getLoadState() == SongLoadState::Loaded) {
    oldSong->unloadAudio();
  }

  loadSongForPerforming(newSong, project, ctx, true);
  preloadNextSong(project, ctx);

  ctx.getEventEngine().fire("setlist.songChanged", ctx);
  broadcast::send(ctx.getWebSocketServer(), "setlist.songChanged",
                  {{"currentIndex", nextIdx}, {"song", newSong->toJson()}});
}

REGISTER_COMMAND("session.advance", SessionAdvanceCommand);

// ── SessionPreviousCommand ───────────────────────────────────────────────────

void SessionPreviousCommand::execute(AppContext& ctx) {
  auto& session = ctx.getSession();
  auto* project = session.getProject();
  if (!project || !session.isLive()) return;

  auto setlistId = session.getPerformingSetlistId();
  if (!setlistId) return;

  const Setlist* setlist = project->setlists().findById(*setlistId);
  if (!setlist) return;

  int currentIdx = session.getPerformingIndex();
  int prevIdx = currentIdx - 1;
  if (prevIdx < 0) return;

  const auto& entries = setlist->getEntries();
  Song* oldSong = project->songs().getSongById(entries[currentIdx].songId);

  // Go back in session index — fires listener (session.changed)
  if (!session.previousPerforming()) return;

  Song* newSong = project->songs().getSongById(entries[prevIdx].songId);
  if (!newSong) return;

  ctx.getAudioEngine().stop();

  if (oldSong && oldSong != newSong &&
      oldSong != ctx.getSongPreloader().getPreloadedSong() &&
      oldSong->getLoadState() == SongLoadState::Loaded) {
    oldSong->unloadAudio();
  }

  loadSongForPerforming(newSong, project, ctx, true);
  preloadNextSong(project, ctx);

  ctx.getEventEngine().fire("setlist.songChanged", ctx);
  broadcast::send(ctx.getWebSocketServer(), "setlist.songChanged",
                  {{"currentIndex", prevIdx}, {"song", newSong->toJson()}});
}

REGISTER_COMMAND("session.previous", SessionPreviousCommand);

// ── SessionGoToCommand ───────────────────────────────────────────────────────

SessionGoToCommand::SessionGoToCommand(int index) : index(index) {}

void SessionGoToCommand::execute(AppContext& ctx) {
  auto& session = ctx.getSession();
  auto* project = session.getProject();
  if (!project || !session.isLive()) return;

  auto setlistId = session.getPerformingSetlistId();
  if (!setlistId) return;

  const Setlist* setlist = project->setlists().findById(*setlistId);
  if (!setlist) return;

  const auto& entries = setlist->getEntries();
  if (index < 0 || index >= static_cast<int>(entries.size())) return;

  int currentIdx = session.getPerformingIndex();
  Song* oldSong = project->songs().getSongById(entries[currentIdx].songId);

  // Jump to index — fires listener (session.changed)
  if (!session.setPerformingIndex(index)) return;

  Song* newSong = project->songs().getSongById(entries[index].songId);
  if (!newSong) return;

  ctx.getAudioEngine().stop();

  if (oldSong && oldSong != newSong &&
      oldSong != ctx.getSongPreloader().getPreloadedSong() &&
      oldSong->getLoadState() == SongLoadState::Loaded) {
    oldSong->unloadAudio();
  }

  loadSongForPerforming(newSong, project, ctx, true);
  preloadNextSong(project, ctx);

  ctx.getEventEngine().fire("setlist.songChanged", ctx);
  broadcast::send(ctx.getWebSocketServer(), "setlist.songChanged",
                  {{"currentIndex", index}, {"song", newSong->toJson()}});
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
    auto* project = session.getProject();
    if (project) {
      if (auto id = session.getPerformingSetlistId()) {
        const Setlist* setlist = project->setlists().findById(*id);
        if (setlist) {
          payload["setlist"] = setlist->toJson();
          payload["currentIndex"] = session.getPerformingIndex();
        }
      }
    }
  }

  response.merge_patch(payload);
  reply(response.dump());
}

REGISTER_COMMAND("session.get", GetSessionCommand);
