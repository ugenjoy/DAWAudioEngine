#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "events/event-rule.hpp"
#include "services/setlist-manager.hpp"
#include "services/songs-manager.hpp"

/**
 * Represents a DAW project.
 *
 * Instances are created exclusively through the static factory methods:
 *   - createNew()    — brand-new project, writes initial structure to disk.
 *   - loadFromDisk() — full load (songs metadata + managers wired up).
 *   - readMetadata() — lightweight snapshot for directory listings; managers
 *                      are NOT initialised.
 *
 * The Project owns its SongsManager and SetlistManager. Callers that need to
 * mutate songs or setlists should obtain a reference via songs() / setlists().
 */
class Project {
 public:
  // Non-copyable, movable.
  Project(const Project&) = delete;
  Project& operator=(const Project&) = delete;
  Project(Project&&) = default;
  Project& operator=(Project&&) = default;

  // ---------------------------------------------------------------------------
  // Factories
  // ---------------------------------------------------------------------------

  /** Create a brand-new project under parentDir and return it. */
  [[nodiscard]] static std::unique_ptr<Project> createNew(std::string name,
                                                          std::string parentDir);

  /** Load a full project from its directory path (no audio loaded). */
  [[nodiscard]] static std::unique_ptr<Project> loadFromDisk(std::string path);

  /**
   * Read only the identity fields from a project directory.
   * Returns a value (not unique_ptr) because it is a lightweight snapshot
   * used for directory listings. SongsManager and SetlistManager are not
   * initialised.
   */
  static Project readMetadata(std::string path);

  // ---------------------------------------------------------------------------
  // Persistence
  // ---------------------------------------------------------------------------

  /** Serialise the project to disk. Returns false on failure. */
  [[nodiscard]] bool save();

  /** Human-readable description of the last error (empty if none). */
  const std::string& getLastError() const;

  // ---------------------------------------------------------------------------
  // Identity
  // ---------------------------------------------------------------------------

  const std::string& getId() const;
  const std::string& getName() const;
  const std::string& getPath() const;
  const std::string& getLastModified() const;

  /** Convenience: returns path + "/audio". */
  std::string getAudioDir() const;

  // ---------------------------------------------------------------------------
  // Contents
  // ---------------------------------------------------------------------------

  SongsManager& songs();
  const SongsManager& songs() const;
  SetlistManager& setlists();
  const SetlistManager& setlists() const;

  const std::vector<EventRule>& getEventRules() const;
  void setEventRules(std::vector<EventRule> rules);

  // ---------------------------------------------------------------------------
  // Serialisation
  // ---------------------------------------------------------------------------

  nlohmann::json toJson() const;

 private:
  /** Default-constructible so that factories can populate fields step by step. */
  Project() = default;

  std::string id;
  std::string name;
  std::string path;
  std::string lastModified;
  std::string lastError;

  std::unique_ptr<SongsManager> songsManager;
  std::unique_ptr<SetlistManager> setlistManager;

  /** Project-level event rules (not tied to any individual song). */
  std::vector<EventRule> projectEventRules;
};
