#include "services/project-manager.hpp"

#include <fstream>

ProjectManager::ProjectManager() : lastError("") {}

ProjectManager::~ProjectManager() = default;

bool ProjectManager::createProject(const std::string& projectPath,
                                   const std::string& name,
                                   const SongsManager& songsManager,
                                   const SetlistManager* setlistManager) {
  // Reset current project metadata so saveProject persists the given name
  // alongside a freshly generated UUID.
  currentProject = Project{};
  currentProject.name = name;
  return saveProject(projectPath, songsManager, setlistManager);
}

bool ProjectManager::saveProject(const std::string& projectPath,
                                 const SongsManager& songsManager,
                                 const SetlistManager* setlistManager) {
  lastError = "";

  // Create JUCE File object for the project folder
  juce::File projectFolder(projectPath);

  // Ensure the path ends with .dawproj
  if (!projectFolder.getFileName().endsWith(".dawproj")) {
    lastError = "Project path must end with .dawproj extension";
    return false;
  }

  // Create project directory structure
  if (!createProjectStructure(projectFolder)) {
    return false;
  }

  // Generate a UUID if the project doesn't have one yet
  if (currentProject.id.empty()) {
    currentProject.id = juce::Uuid().toString().toStdString();
  }
  currentProject.path = projectPath;

  // Serialize the project to JSON
  nlohmann::json projectJson = serializeProject(songsManager, setlistManager);

  // Write JSON to project.json file
  juce::File projectFile = projectFolder.getChildFile("project.json");

  try {
    std::string jsonString = projectJson.dump(2);  // Pretty print with 2 spaces

    if (!projectFile.replaceWithText(jsonString)) {
      lastError = "Failed to write project.json file";
      return false;
    }

  } catch (const std::exception& e) {
    lastError = std::string("JSON serialization error: ") + e.what();
    return false;
  }

  return true;
}

bool ProjectManager::loadProject(const std::string& projectPath,
                                 SongsManager& songsManager,
                                 SetlistManager* setlistManager) {
  lastError = "";

  // Create JUCE File object for the project folder
  juce::File projectFolder(projectPath);

  // Check if project folder exists
  if (!projectFolder.exists() || !projectFolder.isDirectory()) {
    lastError = "Project folder does not exist or is not a directory";
    return false;
  }

  // Check for project.json file
  juce::File projectFile = projectFolder.getChildFile("project.json");
  if (!projectFile.exists()) {
    lastError = "project.json file not found in project folder";
    return false;
  }

  // Read JSON file
  try {
    std::string jsonString = projectFile.loadFileAsString().toStdString();
    nlohmann::json projectJson = nlohmann::json::parse(jsonString);

    // Deserialize the project
    deserializeProject(projectJson, songsManager, setlistManager);

  } catch (const nlohmann::json::parse_error& e) {
    lastError = std::string("JSON parse error: ") + e.what();
    return false;
  } catch (const std::exception& e) {
    lastError = std::string("Error loading project: ") + e.what();
    return false;
  }

  currentProject.path = projectPath;

  // Derive name from folder if not set during deserialization (legacy projects)
  if (currentProject.name.empty()) {
    currentProject.name =
        projectFolder.getFileNameWithoutExtension().toStdString();
  }

  return true;
}

bool ProjectManager::createProjectStructure(const juce::File& projectFolder) {
  // Create main project folder
  if (!projectFolder.exists()) {
    juce::Result result = projectFolder.createDirectory();
    if (result.failed()) {
      lastError = "Failed to create project folder: " +
                  result.getErrorMessage().toStdString();
      return false;
    }
  }

  // Create audio subdirectory for future audio samples
  juce::File audioFolder = projectFolder.getChildFile("audio");
  if (!audioFolder.exists()) {
    juce::Result result = audioFolder.createDirectory();
    if (result.failed()) {
      lastError = "Failed to create audio folder: " +
                  result.getErrorMessage().toStdString();
      return false;
    }
  }

  // Create cache subdirectory for future cached data
  juce::File cacheFolder = projectFolder.getChildFile("cache");
  if (!cacheFolder.exists()) {
    juce::Result result = cacheFolder.createDirectory();
    if (result.failed()) {
      lastError = "Failed to create cache folder: " +
                  result.getErrorMessage().toStdString();
      return false;
    }
  }

  return true;
}

nlohmann::json ProjectManager::serializeProject(
    const SongsManager& songsManager,
    const SetlistManager* setlistManager) const {
  // Build a transient Project view with pointers into the managers, then
  // rely on Project::toJson for the core payload.
  Project view;
  view.id = currentProject.id;
  view.name = currentProject.name;
  view.path = currentProject.path;
  view.songs = const_cast<SongsManager&>(songsManager).getSongList();
  if (setlistManager) {
    view.setlists = const_cast<SetlistManager*>(setlistManager)->getList();
  }

  nlohmann::json projectJson = view.toJson();

  // `path` is a filesystem concern — don't persist it inside project.json so
  // the project folder stays portable across machines.
  projectJson.erase("path");

  projectJson["version"] = "1.0.0";
  projectJson["events"] = songsManager.projectEventsToJson();

  // Strip waveform data from clips (regenerated on load from audio files)
  if (projectJson.contains("songs")) {
    for (auto& song : projectJson["songs"]) {
      if (song.contains("tracks")) {
        for (auto& track : song["tracks"]) {
          if (track.contains("clips")) {
            for (auto& clip : track["clips"]) {
              clip.erase("waveform");
            }
          }
        }
      }
    }
  }

  return projectJson;
}

std::string ProjectManager::getDefaultProjectsDirectory() {
  juce::File home =
      juce::File::getSpecialLocation(juce::File::userHomeDirectory);
  return home.getChildFile("daw/projects").getFullPathName().toStdString();
}

nlohmann::json ProjectManager::listProjects(const std::string& directory) {
  nlohmann::json projects = nlohmann::json::array();

  juce::File dir(directory);
  if (!dir.exists() || !dir.isDirectory()) {
    return projects;
  }

  auto children =
      dir.findChildFiles(juce::File::findDirectories, false, "*.dawproj");

  for (const auto& child : children) {
    nlohmann::json project = getProject(child.getFullPathName().toStdString());
    projects.push_back(project);
  }

  return projects;
}

nlohmann::json ProjectManager::getProject(const std::string& path) {
  nlohmann::json project;

  juce::File dir(path);
  if (!dir.exists() || !dir.isDirectory()) {
    return project;
  }

  const std::string folderName =
      dir.getFileNameWithoutExtension().toStdString();
  const std::string fullPath = dir.getFullPathName().toStdString();

  juce::File projectFile = dir.getChildFile("project.json");
  if (!projectFile.existsAsFile()) {
    project["id"] = nullptr;
    project["name"] = folderName;
    project["path"] = fullPath;
    project["lastModified"] = nullptr;
    project["songs"] = nlohmann::json::array();
    return project;
  }

  project["lastModified"] =
      projectFile.getLastModificationTime().toISO8601(true).toStdString();

  try {
    std::string jsonString = projectFile.loadFileAsString().toStdString();
    nlohmann::json projectJson = nlohmann::json::parse(jsonString);

    Project meta = Project::fromJson(projectJson);
    project["id"] = meta.id.empty() ? nlohmann::json(nullptr)
                                    : nlohmann::json(meta.id);
    project["name"] = meta.name.empty() ? folderName : meta.name;
    project["path"] = fullPath;
    project["songs"] = projectJson.value("songs", nlohmann::json::array());
  } catch (...) {
    project["id"] = nullptr;
    project["name"] = folderName;
    project["path"] = fullPath;
    project["songs"] = nlohmann::json::array();
  }

  return project;
}

void ProjectManager::deserializeProject(const nlohmann::json& projectJson,
                                        SongsManager& songsManager,
                                        SetlistManager* setlistManager) {
  // Parse project metadata (id, name) from the stored JSON.
  Project meta = Project::fromJson(projectJson);

  // Fall back to a freshly generated UUID for legacy files missing an id.
  if (meta.id.empty()) {
    meta.id = juce::Uuid().toString().toStdString();
  }

  currentProject = std::move(meta);

  // Check version (for future compatibility)
  if (projectJson.contains("version")) {
    std::string version = projectJson["version"];
    // Future version checking logic can go here
  }

  // Load songs
  if (projectJson.contains("songs")) {
    songsManager.loadFromJson(projectJson["songs"]);
  }

  // Load project-level events
  if (projectJson.contains("events")) {
    songsManager.loadProjectEventsFromJson(projectJson["events"]);
  }

  if (setlistManager) {
    setlistManager->loadFromJson(
        projectJson.value("setlists", nlohmann::json::array()));
  }
}
