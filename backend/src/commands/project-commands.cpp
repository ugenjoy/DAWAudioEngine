#include "commands/project-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "events/event-engine.hpp"
#include "model/song.hpp"
#include "services/mode-manager.hpp"
#include "services/project-manager.hpp"
#include "services/song-preloader.hpp"
#include "services/songs-manager.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── CreateProjectCommand ───────────────────────────────────────────────────────

CreateProjectCommand::CreateProjectCommand(std::string name)
    : name(std::move(name)) {}

void CreateProjectCommand::execute(AppContext& ctx) {
  auto& projectManager = ctx.getProjectManager();
  auto& songsManager = ctx.getSongsManager();
  auto& audioEngine = ctx.getAudioEngine();
  auto& wsServer = ctx.getWebSocketServer();

  // Stop playback and detach any loaded song
  audioEngine.stop();
  audioEngine.unloadSong();

  // Reset managers to an empty state
  songsManager.loadFromJson(nlohmann::json::array());
  songsManager.setProjectEventRules({});
  ctx.getSetlistManager().clear();
  ctx.getEventEngine().loadRules({}, {}, {});
  ctx.getLoopManager().reset();

  // Build the project path: ~/daw/projects/<name>.dawproj
  std::string projectsDir = ProjectManager::getDefaultProjectsDirectory();
  juce::File dir(projectsDir);
  if (!dir.exists()) dir.createDirectory();

  std::string projectPath = projectsDir + "/" + name + ".dawproj";

  if (projectManager.createProject(projectPath, name, songsManager,
                                   &ctx.getSetlistManager())) {
    juce::Logger::writeToLog("[CreateProjectCommand] Project created: " +
                             juce::String(projectPath));

    Project project = projectManager.getProject(projectPath);
    broadcast::send(wsServer, "project.created",
                    {{"project", project.toJson()},
                     {"setlists", ctx.getSetlistManager().toJson()}});
  } else {
    juce::Logger::writeToLog("[CreateProjectCommand] Failed to create project: " +
                             juce::String(projectManager.getLastError()));

    broadcast::send(wsServer, "project.createFailed",
                    {{"name", name}, {"error", projectManager.getLastError()}});
  }
}

REGISTER_COMMAND_WITH_CREATOR("project.create", CreateProject,
                              [](const nlohmann::json& payload) -> CommandPtr {
                                std::string name = payload.value("name", "");
                                if (name.empty()) return nullptr;
                                return std::make_unique<CreateProjectCommand>(
                                    name);
                              });

// ── LoadProjectCommand ───────────────────────────────────────────────────────

LoadProjectCommand::LoadProjectCommand(std::string projectPath)
    : projectPath(std::move(projectPath)) {}

void LoadProjectCommand::execute(AppContext& ctx) {
  auto& projectManager = ctx.getProjectManager();
  auto& songsManager = ctx.getSongsManager();
  auto& audioEngine = ctx.getAudioEngine();
  auto& wsServer = ctx.getWebSocketServer();

  // Stop playback and detach song before destroying old data
  audioEngine.stop();
  audioEngine.unloadSong();

  // Load the project (metadata only — audio loaded on demand)
  if (projectManager.loadProject(projectPath, songsManager, &ctx.getSetlistManager())) {
    juce::Logger::writeToLog("[LoadProjectCommand] Project loaded: " +
                             juce::String(projectPath));

    // Load first song into audio engine if available
    if (auto* firstSong = songsManager.getSong(0)) {
      const std::string audioDir = projectManager.getAudioDir();

      broadcast::send(wsServer, "song.loading", {{"songId", firstSong->getId()}});
      firstSong->loadAudio(audioDir);

      audioEngine.loadSong(firstSong);
      ctx.getLoopManager().reset();
      ctx.getLoopManager().setLoops(firstSong->getLoops());

      // Load event rules, markers, and fire song.loaded trigger
      ctx.getEventEngine().loadRules(songsManager.getProjectEventRules(), {},
                                     firstSong->getEventRules());
      ctx.getEventEngine().loadMarkers(firstSong->getMarkers());
      ctx.getEventEngine().fire("song.loaded", ctx);

      // Preload next song if in live mode (default mode is Live)
      if (ctx.getModeManager().isLiveMode()) {
        auto songs = songsManager.getSongList();
        ctx.getSongPreloader().onSongChanged(firstSong, songs, audioDir);
      }
    }

    Project project = projectManager.getProject(projectPath);

    broadcast::send(wsServer, "project.loaded",
                    {{"project", project.toJson()},
                     {"setlists", ctx.getSetlistManager().toJson()}});
  } else {
    juce::Logger::writeToLog("[LoadProjectCommand] Failed to load project: " +
                             juce::String(projectManager.getLastError()));

    broadcast::send(wsServer, "project.loadFailed",
                    {{"path", projectPath}, {"error", projectManager.getLastError()}});
  }
}

REGISTER_COMMAND_WITH_CREATOR("project.load", LoadProject,
                              [](const nlohmann::json& payload) -> CommandPtr {
                                std::string path = payload.value("path", "");
                                if (path.empty()) return nullptr;
                                return std::make_unique<LoadProjectCommand>(
                                    path);
                              });

// ── SaveProjectCommand ───────────────────────────────────────────────────────

SaveProjectCommand::SaveProjectCommand(std::string projectPath)
    : projectPath(std::move(projectPath)) {}

void SaveProjectCommand::execute(AppContext& ctx) {
  auto& projectManager = ctx.getProjectManager();
  auto& songsManager = ctx.getSongsManager();

  if (projectManager.saveProject(projectPath, songsManager, &ctx.getSetlistManager())) {
    juce::Logger::writeToLog("[SaveProjectCommand] Project saved: " +
                             juce::String(projectPath));

    broadcast::send(ctx.getWebSocketServer(), "project.saved",
                    {{"path", projectPath}});
  } else {
    juce::Logger::writeToLog("[SaveProjectCommand] Failed to save project: " +
                             juce::String(projectManager.getLastError()));
  }
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "project.save", SaveProject,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string path = payload.value("path", "");
      if (path.empty()) return nullptr;
      return std::make_unique<SaveProjectCommand>(path);
    });

// ── GetLoadedProjectCommand ──────────────────────────────────────────────────

void GetLoadedProjectCommand::execute(AppContext& ctx) {
  auto& projectManager = ctx.getProjectManager();
  auto& audioEngine = ctx.getAudioEngine();

  Project project =
      projectManager.getProject(projectManager.getCurrentProjectPath());

  // Include active song in edit mode (for route restore) or during a live session
  auto& liveSetlist = ctx.getLiveSetlistManager();
  bool isLive = liveSetlist.isActive();
  bool includeActiveSong = isLive || ctx.getModeManager().isEditMode();
  Song* activeSong = includeActiveSong ? audioEngine.getActiveSong() : nullptr;
  nlohmann::json songJson = nullptr;
  if (activeSong != nullptr) songJson = activeSong->toJson();

  // Use the live setlist state (not ModeManager) to determine the restore mode.
  // ModeManager may have been reset to Edit by the frontend navigate-to-/
  // before this command was processed, whereas liveSetlist.isActive() reliably
  // reflects whether a live session is in progress.
  std::string mode = isLive ? "live" : "edit";

  nlohmann::json response;
  response["type"] = "response";
  response["event"] = "project.currentLoaded";
  response["hasProject"] = projectManager.hasLoadedProject();
  response["project"] = project.toJson();
  response["mode"] = mode;
  response["activeSong"] = songJson;
  response["playheadPosition"] = audioEngine.getPlayheadPosition();
  response["cursorPosition"] = audioEngine.getCursorPosition();
  response["isPlaying"] = audioEngine.isPlaying();
  response["masterVolume"] = audioEngine.getMasterVolume();

  // Live session state — allows frontend to restore the /live route after refresh
  if (isLive) {
    response["setlist"] = liveSetlist.getSetlist().toJson();
    response["currentIndex"] = liveSetlist.getCurrentIndex();
  }

  reply(response.dump());

  juce::Logger::writeToLog(
      "[GetLoadedProjectCommand] Current project: " +
      juce::String(projectManager.hasLoadedProject()
                       ? projectManager.getCurrentProjectPath()
                       : "none"));
}

REGISTER_COMMAND("project.getLoaded", GetLoadedProjectCommand);

// ── ListProjectsCommand ──────────────────────────────────────────────────────

void ListProjectsCommand::execute(AppContext& ctx) {
  auto& projectManager = ctx.getProjectManager();

  std::string directory = ProjectManager::getDefaultProjectsDirectory();

  // Create directory if it doesn't exist
  juce::File dir(directory);
  if (!dir.exists()) {
    dir.createDirectory();
  }

  std::vector<Project> projects = projectManager.listProjects(directory);

  nlohmann::json projectsJson = nlohmann::json::array();
  for (const auto& p : projects) {
    projectsJson.push_back(p.toJson());
  }

  nlohmann::json response;
  response["type"] = "response";
  response["event"] = "project.listed";
  response["directory"] = directory;
  response["projects"] = projectsJson;

  reply(response.dump());

  juce::Logger::writeToLog("[ListProjectsCommand] Listed " +
                           juce::String((int)projects.size()) +
                           " projects from " + juce::String(directory));
}

REGISTER_COMMAND("project.list", ListProjectsCommand);

// ── LoadSongCommand ──────────────────────────────────────────────────────────

LoadSongCommand::LoadSongCommand(std::string uuid) : uuid(std::move(uuid)) {}

void LoadSongCommand::execute(AppContext& ctx) {
  auto& songsManager = ctx.getSongsManager();
  auto& audioEngine = ctx.getAudioEngine();
  auto& projectManager = ctx.getProjectManager();
  auto& wsServer = ctx.getWebSocketServer();

  Song* activeSong = audioEngine.getActiveSong();

  std::vector<Song*> songs = songsManager.getSongList();
  Song* nextSong = nullptr;

  for (int i = 0; i < static_cast<int>(songs.size()); i++) {
    if (uuid == songs[i]->getId() && songs[i] != activeSong) {
      nextSong = songs[i];
    }
  }

  if (nextSong == nullptr) {
    juce::Logger::writeToLog("[LoadSongCommand] nextSong not found or already active");
    return;
  }

  // Stop playback and reset position before swapping songs
  audioEngine.stop();
  audioEngine.setPlayheadPosition(0.0);
  audioEngine.setCursorPosition(0.0);

  const std::string audioDir = projectManager.getAudioDir();

  // Load audio for the new song if not already loaded
  if (nextSong->getLoadState() != SongLoadState::Loaded) {
    broadcast::send(wsServer, "song.loading", {{"songId", nextSong->getId()}});
    nextSong->loadAudio(audioDir);
  }

  // Unload audio from the previous song
  if (activeSong != nullptr) {
    activeSong->unloadAudio();
    broadcast::send(wsServer, "song.unloaded", {{"songId", activeSong->getId()}});
  }

  audioEngine.loadSong(nextSong);
  ctx.getLoopManager().reset();
  ctx.getLoopManager().setLoops(nextSong->getLoops());

  // Load event rules, markers, and fire song.loaded trigger
  ctx.getEventEngine().loadRules(songsManager.getProjectEventRules(), {},
                                 nextSong->getEventRules());
  ctx.getEventEngine().loadMarkers(nextSong->getMarkers());
  ctx.getEventEngine().fire("song.loaded", ctx);

  // Preload next song if in live mode
  if (ctx.getModeManager().isLiveMode()) {
    auto songs = ctx.getSongsManager().getSongList();
    ctx.getSongPreloader().onSongChanged(nextSong, songs, audioDir);
  }
}

REGISTER_COMMAND_WITH_CREATOR("project.loadSong", LoadSong,
                              [](const nlohmann::json& payload) -> CommandPtr {
                                std::string uuid = payload.value("uuid", "");
                                if (uuid.empty()) return nullptr;
                                return std::make_unique<LoadSongCommand>(uuid);
                              });