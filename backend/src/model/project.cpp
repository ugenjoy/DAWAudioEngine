#include "model/project.hpp"

#include "model/setlist.hpp"
#include "model/song.hpp"

nlohmann::json Project::toJson() const {
  nlohmann::json songsJson = nlohmann::json::array();
  if (!songs.empty()) {
    for (const auto* s : songs) {
      if (s) songsJson.push_back(s->toJson());
    }
  } else {
    for (const auto& s : ownedSongs) {
      if (s) songsJson.push_back(s->toJson());
    }
  }

  nlohmann::json setlistsJson = nlohmann::json::array();
  for (const auto* sl : setlists) {
    if (sl) setlistsJson.push_back(sl->toJson());
  }

  return {
      {"id", id},
      {"name", name},
      {"path", path},
      {"lastModified", lastModified},
      {"songs", songsJson},
      {"setlists", setlistsJson},
  };
}

Project Project::fromJson(const nlohmann::json& j) {
  Project p;
  p.id = j.value("id", "");
  p.name = j.value("name", "");
  p.path = j.value("path", "");
  p.lastModified = j.value("lastModified", "");

  // Parse songs as owned metadata-only instances (no audio loaded).
  // Callers working with the currently-loaded project should instead route
  // songs through SongsManager and attach pointers to `songs` directly.
  if (j.contains("songs") && j["songs"].is_array()) {
    for (const auto& songJson : j["songs"]) {
      p.ownedSongs.push_back(Song::fromJson(songJson, "", false));
    }
  }

  return p;
}
