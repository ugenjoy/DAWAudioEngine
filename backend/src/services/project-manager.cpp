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

  // Refresh lastModified from the freshly written file
  currentProject.lastModified =
      projectFile.getLastModificationTime().toISO8601(true).toStdString();

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
  currentProject.lastModified =
      projectFile.getLastModificationTime().toISO8601(true).toStdString();

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
  // Borrow manager-owned song/setlist pointers into an in-memory Project view
  // and delegate the core payload to Project::toJson.
  Project view;
  view.id = currentProject.id;
  view.name = currentProject.name;
  view.path = currentProject.path;
  view.lastModified = currentProject.lastModified;
  view.songs = const_cast<SongsManager&>(songsManager).getSongList();
  if (setlistManager) {
    view.setlists = const_cast<SetlistManager*>(setlistManager)->getList();
  }

  nlohmann::json projectJson = view.toJson();

  // `path` and `lastModified` are filesystem concerns — don't persist them
  // inside project.json so the folder stays portable across machines and the
  // timestamp always reflects the actual on-disk file.
  projectJson.erase("path");
  projectJson.erase("lastModified");

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

std::vector<Project> ProjectManager::listProjects(
    const std::string& directory) {
  std::vector<Project> projects;

  juce::File dir(directory);
  if (!dir.exists() || !dir.isDirectory()) {
    return projects;
  }

  auto children =
      dir.findChildFiles(juce::File::findDirectories, false, "*.dawproj");

  projects.reserve(static_cast<size_t>(children.size()));
  for (const auto& child : children) {
    projects.push_back(getProject(child.getFullPathName().toStdString()));
  }

  return projects;
}

Project ProjectManager::getProject(const std::string& path) {
  Project project;

  juce::File dir(path);
  if (!dir.exists() || !dir.isDirectory()) {
    return project;
  }

  const std::string folderName =
      dir.getFileNameWithoutExtension().toStdString();
  const std::string fullPath = dir.getFullPathName().toStdString();

  juce::File projectFile = dir.getChildFile("project.json");
  if (!projectFile.existsAsFile()) {
    // Folder exists but no project.json — return a minimal placeholder.
    project.name = folderName;
    project.path = fullPath;
    return project;
  }

  try {
    std::string jsonString = projectFile.loadFileAsString().toStdString();
    nlohmann::json projectJson = nlohmann::json::parse(jsonString);
    project = Project::fromJson(projectJson);
  } catch (...) {
    // Corrupted project.json — fall through with metadata fallback.
    project = Project{};
  }

  project.path = fullPath;
  if (project.name.empty()) project.name = folderName;
  project.lastModified =
      projectFile.getLastModificationTime().toISO8601(true).toStdString();

  return project;
}

void ProjectManager::deserializeProject(const nlohmann::json& projectJson,
                                        SongsManager& songsManager,
                                        SetlistManager* setlistManager) {
  // Parse project metadata (id, name) from the stored JSON. `ownedSongs` is
  // populated by Project::fromJson but discarded here — the real Song
  // instances live in SongsManager once `loadFromJson` below has run.
  Project meta = Project::fromJson(projectJson);
  meta.ownedSongs.clear();

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
