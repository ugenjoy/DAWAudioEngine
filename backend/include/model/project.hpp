#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "model/setlist.hpp"
#include "model/song.hpp"

/**
 * Represents a daw project.
 *
 * A Project instance serves two use cases:
 *
 * 1. **Currently-loaded view** — `songs`/`setlists` hold non-owning pointers
 *    into SongsManager/SetlistManager storage. `ownedSongs` stays empty.
 *
 * 2. **Standalone snapshot** — built from a project.json on disk (e.g. via
 *    ProjectManager::getProject). `ownedSongs` owns metadata-only Song
 *    instances (no audio loaded). The non-owning `songs` vector stays empty.
 *
 * `toJson()` serializes whichever is populated.
 */
struct Project {
  std::string id;             // UUID
  std::string name;
  std::string path;
  std::string lastModified;   // ISO8601 timestamp, empty if not yet persisted

  std::vector<Song*> songs;
  std::vector<Setlist*> setlists;

  // Owned metadata-only songs (used when the Project stands alone).
  std::vector<std::unique_ptr<Song>> ownedSongs;

  nlohmann::json toJson() const;
  static Project fromJson(const nlohmann::json& j);
};
