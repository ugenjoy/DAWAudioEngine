#include "commands/project-commands.hpp"
#include <nlohmann/json.hpp>
#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
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

  // Stop playback before loading
  audioEngine.stop();

  // Load the project
  if (projectManager.loadProject(projectPath, songsManager)) {
    juce::Logger::writeToLog("[LoadProjectCommand] Project loaded: " +
                             juce::String(projectPath));

    // Load first song into audio engine if available
    if (auto* firstSong = songsManager.getSong(0)) {
      audioEngine.loadSong(firstSong);
      juce::Logger::writeToLog("[LoadProjectCommand] First song loaded");
    }

    // Broadcast project loaded event to all clients
    nlohmann::json broadcast;
    broadcast["type"] = "broadcast";
    broadcast["event"] = "project.loaded";
    broadcast["path"] = projectPath;
    broadcast["songsCount"] = (int)songsManager.getSongList().size();

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
  } else {
    juce::Logger::writeToLog("[SaveProjectCommand] Failed to save project: " +
                             juce::String(projectManager.getLastError()));
  }
}

void GetLoadedProjectCommand::execute(AppContext& ctx) {
  auto& projectManager = ctx.getProjectManager();
  auto& wsServer = ctx.getWebSocketServer();

  nlohmann::json response;
  response["type"] = "broadcast";
  response["event"] = "project.currentLoaded";
  response["hasProject"] = projectManager.hasLoadedProject();
  response["path"] = projectManager.getCurrentProjectPath();

  wsServer.broadcast(response.dump());

  juce::Logger::writeToLog(
      "[GetLoadedProjectCommand] Current project: " +
      juce::String(projectManager.hasLoadedProject()
                       ? projectManager.getCurrentProjectPath()
                       : "none"));
}

// Auto-registration
static CommandRegistrar registerLoadProject(
    "project.load",
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string path = payload.value("path", "");
      if (path.empty()) {
        return nullptr;
      }
      return std::make_unique<LoadProjectCommand>(path);
    });

static CommandRegistrar registerSaveProject(
    "project.save",
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string path = payload.value("path", "");
      if (path.empty()) {
        return nullptr;
      }
      return std::make_unique<SaveProjectCommand>(path);
    });

static CommandRegistrar registerGetLoadedProject(
    "project.getLoaded",
    [](const nlohmann::json& /* payload */) -> CommandPtr {
      return std::make_unique<GetLoadedProjectCommand>();
    });