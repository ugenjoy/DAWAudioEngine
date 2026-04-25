#pragma once

#include <memory>
#include <string>
#include <vector>

#include "model/project.hpp"
#include "services/setlist-manager.hpp"
#include "services/songs-manager.hpp"

/**
 * @file project-manager.hpp
 * @brief Shim over Project that preserves the original ProjectManager public API.
 *
 * Internally holds a std::unique_ptr<Project> and forwards all operations to
 * the Project class. External SongsManager / SetlistManager are still supported
 * for backward-compatibility during the refactor (Task 1.5). They will be
 * removed in Task 1.6.
 */

/**
 * @class ProjectManager
 * @brief Shim that delegates project I/O to the Project class while keeping
 *        the original public API unchanged.
 */
class ProjectManager {
 public:
  ProjectManager();
  ~ProjectManager();

  /**
   * @brief Create a new project on disk with a specific name.
   *
   * @param projectPath Absolute path to the project folder (must end with .dawproj)
   * @param name Human-readable project name
   * @param songsManager Songs to serialize (typically empty on creation)
   * @param setlistManager Optional setlists to serialize
   * @return true if the project was created successfully
   */
  bool createProject(const std::string& projectPath, const std::string& name,
                     const SongsManager& songsManager,
                     const SetlistManager* setlistManager = nullptr);

  /**
   * @brief Save the current project to disk.
   *
   * Syncs external managers into the Project's internal managers before
   * delegating to Project::save().
   *
   * @param projectPath Absolute path to the project folder
   * @param songsManager Authoritative songs/events data to persist
   * @param setlistManager Optional authoritative setlists to persist
   * @return true if save was successful, false otherwise
   */
  bool saveProject(const std::string& projectPath,
                   const SongsManager& songsManager,
                   const SetlistManager* setlistManager = nullptr);

  /**
   * @brief Load a project from disk and populate the external managers.
   *
   * @param projectPath Absolute path to the project folder
   * @param songsManager Populated with the loaded songs/events
   * @param setlistManager Optionally populated with the loaded setlists
   * @return true if load was successful, false otherwise
   */
  bool loadProject(const std::string& projectPath,
                   SongsManager& songsManager,
                   SetlistManager* setlistManager = nullptr);

  /**
   * @brief Get the last error message
   * @return String describing the last error that occurred
   */
  std::string getLastError() const { return lastError; }

  /**
   * @brief Access the currently loaded Project (pointer, may be null).
   */
  const Project* getCurrentProject() const { return current.get(); }

  /**
   * @brief Non-const access to the currently loaded Project (for AppContext).
   * @return Pointer to the current Project, or nullptr if none is loaded.
   */
  Project* getCurrentProjectRef() const { return current.get(); }

  /**
   * @brief Get the currently loaded project path
   * @return Path to the loaded project, or empty string if no project is loaded
   */
  std::string getCurrentProjectPath() const;

  /**
   * @brief Check if a project is currently loaded
   * @return true if a project is loaded, false otherwise
   */
  bool hasLoadedProject() const;

  /**
   * @brief Get the UUID of the currently loaded project
   * @return UUID string, or empty string if no project is loaded
   */
  std::string getCurrentProjectId() const;

  /**
   * @brief Get the audio directory for the currently loaded project
   * @return Absolute path to the audio/ subdirectory
   */
  std::string getAudioDir() const;

  /**
   * @brief Get the default projects directory path (~/daw/projects/)
   * @return Absolute path to the default projects directory
   */
  static std::string getDefaultProjectsDirectory();

  /**
   * @brief List all .dawproj projects in a given directory
   * @param directory Path to scan for projects
   * @return Vector of standalone Project snapshots (metadata only)
   */
  std::vector<Project> listProjects(const std::string& directory);

  /**
   * @brief Read a .dawproj project folder as a standalone snapshot
   * @param path Path of the project
   * @return Project populated from project.json (empty Project on error)
   */
  Project getProject(const std::string& path);

 private:
  std::string lastError;
  std::unique_ptr<Project> current;
};
