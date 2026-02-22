#include "commands/project-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "events/event-engine.hpp"
#include "services/project-manager.hpp"
#include "services/songs-manager.hpp"
#include "websocket/websocket-server.hpp"

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

  // Load the project
  if (projectManager.loadProject(projectPath, songsManager)) {
    juce::Logger::writeToLog("[LoadProjectCommand] Project loaded: " +
                             juce::String(projectPath));

    // Load first song into audio engine if available
    if (auto* firstSong = songsManager.getSong(0)) {
      audioEngine.loadSong(firstSong);

      // Load event rules and fire song.loaded trigger
      ctx.getEventEngine().loadRules(songsManager.getProjectEventRules(),
                                     firstSong->getEventRules());
      ctx.getEventEngine().fire("song.loaded", ctx);
    }

    nlohmann::json project = projectManager.getProject(projectPath);
    nlohmann::json songs = songsManager.toJson();

    // Broadcast project loaded event to all clients
    nlohmann::json broadcast;
    broadcast["type"] = "broadcast";
    broadcast["event"] = "project.loaded";
    broadcast["project"] = project;

    wsServer.broadcast(broadcast.dump());
  } else {
    juce::Logger::writeToLog("[LoadProjectCommand] Failed to load project: " +
                             juce::String(projectManager.getLastError()));

    // Broadcast error event
    nlohmann::json broadcast;
    broadcast["type"] = "broadcast";
    broadcast["event"] = "project.loadFailed";
    broadcast["path"] = projectPath;
    broadcast["error"] = projectManager.getLastError();

    wsServer.broadcast(broadcast.dump());
  }
}

SaveProjectCommand::SaveProjectCommand(std::string projectPath)
    : projectPath(std::move(projectPath)) {}

void SaveProjectCommand::execute(AppContext& ctx) {
  auto& projectManager = ctx.getProjectManager();
  auto& songsManager = ctx.getSongsManager();

  if (projectManager.saveProject(projectPath, songsManager)) {
    juce::Logger::writeToLog("[SaveProjectCommand] Project saved: " +
                             juce::String(projectPath));

    nlohmann::json broadcast;
    broadcast["type"] = "broadcast";
    broadcast["event"] = "project.saved";
    broadcast["path"] = projectPath;
    ctx.getWebSocketServer().broadcast(broadcast.dump());
  } else {
    juce::Logger::writeToLog("[SaveProjectCommand] Failed to save project: " +
                             juce::String(projectManager.getLastError()));
  }
}

void GetLoadedProjectCommand::execute(AppContext& ctx) {
  auto& projectManager = ctx.getProjectManager();
  auto& audioEngine = ctx.getAudioEngine();

  nlohmann::json project =
      projectManager.getProject(projectManager.getCurrentProjectPath());

  Song* activeSong = audioEngine.getActiveSong();
  nlohmann::json songJson = "";

  if (activeSong != nullptr) {
    songJson = activeSong->toJson();
  }

  nlohmann::json response;
  response["type"] = "response";
  response["event"] = "project.currentLoaded";
  response["hasProject"] = projectManager.hasLoadedProject();
  response["project"] = project;
  response["activeSong"] = songJson;
  response["playheadPosition"] = audioEngine.getPlayheadPosition();
  response["cursorPosition"] = audioEngine.getCursorPosition();
  response["isPlaying"] = audioEngine.isPlaying();

  reply(response.dump());

  juce::Logger::writeToLog(
      "[GetLoadedProjectCommand] Current project: " +
      juce::String(projectManager.hasLoadedProject()
                       ? projectManager.getCurrentProjectPath()
                       : "none"));
}

void ListProjectsCommand::execute(AppContext& ctx) {
  auto& projectManager = ctx.getProjectManager();

  std::string directory = ProjectManager::getDefaultProjectsDirectory();

  // Create directory if it doesn't exist
  juce::File dir(directory);
  if (!dir.exists()) {
    dir.createDirectory();
  }

  nlohmann::json projects = projectManager.listProjects(directory);

  nlohmann::json response;
  response["type"] = "response";
  response["event"] = "project.listed";
  response["directory"] = directory;
  response["projects"] = projects;

  reply(response.dump());

  juce::Logger::writeToLog("[ListProjectsCommand] Listed " +
                           juce::String((int)projects.size()) +
                           " projects from " + juce::String(directory));
}

LoadSongCommand::LoadSongCommand(std::string uuid) : uuid(std::move(uuid)) {}

void LoadSongCommand::execute(AppContext& ctx) {
  auto& songsManager = ctx.getSongsManager();
  auto& audioEngine = ctx.getAudioEngine();

  Song* activeSong = audioEngine.getActiveSong();

  if (activeSong == nullptr) {
    juce::Logger::writeToLog("[LoadSongCommand] activeSong is null");
    return;
  }
  std::vector<Song*> songs = songsManager.getSongList();
  Song* nextSong = nullptr;

  for (int i = 0; i < songs.size(); i++) {
    if (uuid == songs[i]->getId() && songs[i] != activeSong) {
      nextSong = songs[i];
    }
  }

  if (nextSong == nullptr) {
    juce::Logger::writeToLog("[LoadSongCommand] nextSong is null");
    return;
  }

  audioEngine.loadSong(nextSong);

  // Load event rules and fire song.loaded trigger
  ctx.getEventEngine().loadRules(songsManager.getProjectEventRules(),
                                 nextSong->getEventRules());
  ctx.getEventEngine().fire("song.loaded", ctx);
}

// Auto-registration
REGISTER_COMMAND_WITH_CREATOR("project.load", LoadProject,
                              [](const nlohmann::json& payload) -> CommandPtr {
                                std::string path = payload.value("path", "");
                                if (path.empty()) return nullptr;
                                return std::make_unique<LoadProjectCommand>(
                                    path);
                              });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "project.save", SaveProject,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string path = payload.value("path", "");
      if (path.empty()) return nullptr;
      return std::make_unique<SaveProjectCommand>(path);
    });

REGISTER_COMMAND("project.getLoaded", GetLoadedProjectCommand);

REGISTER_COMMAND("project.list", ListProjectsCommand);

REGISTER_COMMAND_WITH_CREATOR("project.loadSong", LoadSong,
                              [](const nlohmann::json& payload) -> CommandPtr {
                                std::string uuid = payload.value("uuid", "");
                                if (uuid.empty()) return nullptr;
                                return std::make_unique<LoadSongCommand>(uuid);
                              });