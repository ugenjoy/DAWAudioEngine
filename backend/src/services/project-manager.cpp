#include "services/project-manager.hpp"
#include <fstream>

ProjectManager::ProjectManager()
    : lastError(""), currentProjectPath(""), currentProjectId("") {}

ProjectManager::~ProjectManager() = default;

bool ProjectManager::saveProject(const std::string& projectPath,
                                 const SongsManager& songsManager) {
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
  if (currentProjectId.empty()) {
    currentProjectId = juce::Uuid().toString().toStdString();
  }

  // Serialize the project to JSON
  nlohmann::json projectJson = serializeProject(songsManager);

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

  currentProjectPath = projectPath;
  return true;
}

bool ProjectManager::loadProject(const std::string& projectPath,
                                 SongsManager& songsManager) {
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
    deserializeProject(projectJson, songsManager);

  } catch (const nlohmann::json::parse_error& e) {
    lastError = std::string("JSON parse error: ") + e.what();
    return false;
  } catch (const std::exception& e) {
    lastError = std::string("Error loading project: ") + e.what();
    return false;
  }

  currentProjectPath = projectPath;
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
    const SongsManager& songsManager) const {
  nlohmann::json projectJson;

  // Project metadata
  projectJson["id"] = currentProjectId;
  projectJson["version"] = "1.0.0";

  // Serialize songs
  projectJson["songs"] = songsManager.toJson();

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

  project["name"] = dir.getFileNameWithoutExtension().toStdString();
  project["path"] = dir.getFullPathName().toStdString();

  juce::File projectFile = dir.getChildFile("project.json");
  if (projectFile.existsAsFile()) {
    // Get last modified time as ISO 8601
    juce::Time modTime = projectFile.getLastModificationTime();
    project["lastModified"] = modTime.toISO8601(true).toStdString();

    // Read project metadata from project.json
    try {
      std::string jsonString = projectFile.loadFileAsString().toStdString();
      nlohmann::json projectJson = nlohmann::json::parse(jsonString);

      if (projectJson.contains("id") && projectJson["id"].is_string()) {
        project["id"] = projectJson["id"].get<std::string>();
      } else {
        project["id"] = nullptr;
      }

      if (projectJson.contains("songs") && projectJson["songs"].is_array()) {
        project["songsCount"] = (int)projectJson["songs"].size();
      } else {
        project["songsCount"] = 0;
      }
    } catch (...) {
      project["id"] = nullptr;
      project["songsCount"] = -1;
    }
  } else {
    project["lastModified"] = nullptr;
    project["songsCount"] = -1;
  }

  return project;
}

void ProjectManager::deserializeProject(const nlohmann::json& projectJson,
                                        SongsManager& songsManager) {
  // Load project UUID, or generate one if missing (backward compatibility)
  if (projectJson.contains("id") && projectJson["id"].is_string()) {
    currentProjectId = projectJson["id"].get<std::string>();
  } else {
    currentProjectId = juce::Uuid().toString().toStdString();
  }

  // Check version (for future compatibility)
  if (projectJson.contains("version")) {
    std::string version = projectJson["version"];
    // Future version checking logic can go here
  }

  // Load songs
  if (projectJson.contains("songs")) {
    songsManager.loadFromJson(projectJson["songs"]);
  }
}
