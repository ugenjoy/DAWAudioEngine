#include "commands/project-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "events/event-engine.hpp"
#include "model/project.hpp"
#include "model/song.hpp"
#include "services/project-repository.hpp"
#include "services/session.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── CreateProjectCommand ───────────────────────────────────────────────────────

CreateProjectCommand::CreateProjectCommand(std::string name)
    : name(std::move(name)) {}

void CreateProjectCommand::execute(AppContext& ctx) {
  auto& wsServer = ctx.getWebSocketServer();

  // Stop playback and detach any loaded song
  ctx.getAudioEngine().stop();
  ctx.getAudioEngine().unloadSong();
  ctx.getEventEngine().loadRules({}, {}, {});
  ctx.getLoopManager().reset();

  std::string projectsDir = ProjectRepository::getDefaultProjectsDirectory();
  juce::File dir(projectsDir);
  if (!dir.exists()) dir.createDirectory();

  auto project = Project::createNew(name, projectsDir);
  if (!project) {
    broadcast::send(wsServer, "project.createFailed", {{"name", name}});
    return;
  }

  const nlohmann::json projectJson = project->toJson();
  ctx.getSession().openProject(std::move(project));

  broadcast::send(wsServer, "project.created", {{"project", projectJson}});
  // session.changed is broadcast by Session listener
}

REGISTER_COMMAND_WITH_CREATOR("project.create", CreateProject,
                              [](const nlohmann::json& payload) -> CommandPtr {
                                std::string name = payload.value("name", "");
                                if (name.empty()) return nullptr;
                                return std::make_unique<CreateProjectCommand>(
                                    name);
                              });

// ── SaveProjectCommand ───────────────────────────────────────────────────────

SaveProjectCommand::SaveProjectCommand(std::string projectPath)
    : projectPath(std::move(projectPath)) {}

void SaveProjectCommand::execute(AppContext& ctx) {
  auto* project = ctx.getSession().getProject();
  if (!project) return;

  if (project->save()) {
    juce::Logger::writeToLog("[SaveProjectCommand] Project saved: " +
                             juce::String(project->getPath()));
    broadcast::send(ctx.getWebSocketServer(), "project.saved",
                    {{"path", project->getPath()}});
  } else {
    juce::Logger::writeToLog("[SaveProjectCommand] Failed to save project: " +
                             juce::String(project->getLastError()));
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
  auto& session = ctx.getSession();
  auto& audioEngine = ctx.getAudioEngine();

  nlohmann::json response;
  response["type"] = "response";
  response["event"] = "project.currentLoaded";

  nlohmann::json sessionJson = session.toJson();
  response.merge_patch(sessionJson);
  response["playback"] = {
      {"isPlaying", audioEngine.isPlaying()},
      {"playheadPosition", audioEngine.getPlayheadPosition()},
      {"cursorPosition", audioEngine.getCursorPosition()},
      {"masterVolume", audioEngine.getMasterVolume()}};

  // Backward compatibility
  response["hasProject"] = session.getProject() != nullptr;
  if (auto* s = session.getFocusedSong()) {
    response["activeSong"] = s->toJson();
  }
  response["mode"] = session.isLive() ? "live" : "edit";

  reply(response.dump());

  juce::Logger::writeToLog(
      "[GetLoadedProjectCommand] Current project: " +
      juce::String(session.getProject() ? session.getProject()->getPath()
                                        : "none"));
}

REGISTER_COMMAND("project.getLoaded", GetLoadedProjectCommand);

// ── ListProjectsCommand ──────────────────────────────────────────────────────

void ListProjectsCommand::execute(AppContext& ctx) {
  std::string directory = ProjectRepository::getDefaultProjectsDirectory();

  // Create directory if it doesn't exist
  juce::File dir(directory);
  if (!dir.exists()) {
    dir.createDirectory();
  }

  auto projects = ProjectRepository::scan(directory);

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
