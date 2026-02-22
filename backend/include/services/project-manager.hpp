#pragma once

#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>
#include <string>
#include "songs-manager.hpp"

/**
 * @file project-manager.hpp
 * @brief Manages project file I/O operations
 */

/**
 * @class ProjectManager
 * @brief Handles saving and loading DAW projects to/from disk
 *
 * Projects are stored as folders with the .dawproj extension containing:
 * - project.json: Main project file with songs and tracks data
 * - audio/: Directory for future audio samples
 * - cache/: Directory for future cached data (waveforms, etc.)
 */
class ProjectManager {
 public:
  ProjectManager();
  ~ProjectManager();

  /**
   * @brief Save the current project to disk
   * @param projectPath Absolute path to the project folder (e.g.,
   * "/path/to/MyProject.dawproj")
   * @param songsManager Reference to the SongsManager containing project data
   * @return true if save was successful, false otherwise
   */
  bool saveProject(const std::string& projectPath,
                   const SongsManager& songsManager);

  /**
   * @brief Load a project from disk
   * @param projectPath Absolute path to the project folder
   * @param songsManager Reference to SongsManager to populate with loaded data
   * @return true if load was successful, false otherwise
   */
  bool loadProject(const std::string& projectPath, SongsManager& songsManager);

  /**
   * @brief Get the last error message
   * @return String describing the last error that occurred
   */
  std::string getLastError() const { return lastError; }

  /**
   * @brief Get the currently loaded project path
   * @return Path to the loaded project, or empty string if no project is loaded
   */
  std::string getCurrentProjectPath() const { return currentProjectPath; }

  /**
   * @brief Check if a project is currently loaded
   * @return true if a project is loaded, false otherwise
   */
  bool hasLoadedProject() const { return !currentProjectPath.empty(); }

  /**
   * @brief Get the UUID of the currently loaded project
   * @return UUID string, or empty string if no project is loaded
   */
  std::string getCurrentProjectId() const { return currentProjectId; }

  /**
   * @brief Get the default projects directory path (~/daw/projects/)
   * @return Absolute path to the default projects directory
   */
  static std::string getDefaultProjectsDirectory();

  /**
   * @brief List all .dawproj projects in a given directory
   * @param directory Path to scan for projects
   * @return JSON array of project metadata (name, path, lastModified,
   * songsCount)
   */
  nlohmann::json listProjects(const std::string& directory);

  /**
   * @brief Get a .dawproj project
   * @param path Path of the project
   * @return JSON of project metadata (name, path, lastModified,
   * songsCount)
   */
  nlohmann::json getProject(const std::string& path);

 private:
  /**
   * @brief Create the project directory structure
   * @param projectFolder JUCE File object for the project folder
   * @return true if creation was successful
   */
  bool createProjectStructure(const juce::File& projectFolder);

  /**
   * @brief Serialize the entire project to JSON
   * @param songsManager The SongsManager to serialize
   * @return JSON object representing the project
   */
  nlohmann::json serializeProject(const SongsManager& songsManager) const;

  /**
   * @brief Deserialize project from JSON
   * @param projectJson The JSON object to deserialize
   * @param songsManager The SongsManager to populate
   */
  void deserializeProject(const nlohmann::json& projectJson,
                          SongsManager& songsManager);

  std::string lastError;
  std::string currentProjectPath;
  std::string currentProjectId;
  std::string currentProjectName;
};
