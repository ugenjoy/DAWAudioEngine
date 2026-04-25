#include "services/project-manager.hpp"

#include <juce_core/juce_core.h>

#include "services/project-repository.hpp"

ProjectManager::ProjectManager() : lastError("") {}

ProjectManager::~ProjectManager() = default;

bool ProjectManager::createProject(const std::string& projectPath,
                                   const std::string& name,
                                   const SongsManager& /*songsManager*/,
                                   const SetlistManager* /*setlistManager*/) {
  lastError = "";

  // Derive the parent directory from the full project path
  juce::File f(projectPath);
  std::string parentDir =
      f.getParentDirectory().getFullPathName().toStdString();

  current = Project::createNew(name, parentDir);
  if (!current) {
    lastError = "Failed to create project";
    return false;
  }

  // External songsManager is empty on creation — no sync needed.
  return true;
}

bool ProjectManager::saveProject(const std::string& /*projectPath*/,
                                 const SongsManager& songsManager,
                                 const SetlistManager* setlistManager) {
  lastError = "";

  if (!current) {
    lastError = "No project loaded";
    return false;
  }

  // Sync external (authoritative) managers into the Project's internal copies.
  current->songs().loadFromJson(songsManager.toJson());
  current->setEventRules(songsManager.getProjectEventRules());
  if (setlistManager) {
    current->setlists().loadFromJson(setlistManager->toJson());
  }

  if (!current->save()) {
    lastError = current->getLastError();
    return false;
  }

  return true;
}

bool ProjectManager::loadProject(const std::string& projectPath,
                                 SongsManager& songsManager,
                                 SetlistManager* setlistManager) {
  lastError = "";

  current = Project::loadFromDisk(projectPath);
  if (!current) {
    lastError = "Failed to load project from: " + projectPath;
    return false;
  }

  // Export loaded data into the external managers for backward-compat.
  songsManager.loadFromJson(current->songs().toJson());
  songsManager.setProjectEventRules(current->getEventRules());
  if (setlistManager) {
    setlistManager->loadFromJson(current->setlists().toJson());
  }

  return true;
}

std::string ProjectManager::getCurrentProjectPath() const {
  return current ? current->getPath() : "";
}

bool ProjectManager::hasLoadedProject() const {
  return current != nullptr && !current->getPath().empty();
}

std::string ProjectManager::getCurrentProjectId() const {
  return current ? current->getId() : "";
}

std::string ProjectManager::getAudioDir() const {
  return current ? current->getAudioDir() : "";
}

std::string ProjectManager::getDefaultProjectsDirectory() {
  return ProjectRepository::getDefaultProjectsDirectory();
}

std::vector<Project> ProjectManager::listProjects(
    const std::string& directory) {
  return ProjectRepository::scan(directory);
}

Project ProjectManager::getProject(const std::string& path) {
  auto p = Project::loadFromDisk(path);
  if (!p) {
    // Fallback to metadata-only snapshot on load failure
    return Project::readMetadata(path);
  }
  return std::move(*p);
}
