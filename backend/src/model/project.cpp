#include "model/project.hpp"

#include "model/setlist.hpp"
#include "model/song.hpp"

nlohmann::json Project::toJson() const {
  nlohmann::json songsJson = nlohmann::json::array();
  for (const auto* s : songs) {
    if (s) songsJson.push_back(s->toJson());
  }

  nlohmann::json setlistsJson = nlohmann::json::array();
  for (const auto* sl : setlists) {
    if (sl) setlistsJson.push_back(sl->toJson());
  }

  return {
      {"id", id},
      {"name", name},
      {"path", path},
      {"songs", songsJson},
      {"setlists", setlistsJson},
  };
}

Project Project::fromJson(const nlohmann::json& j) {
  Project p;
  p.id = j.value("id", "");
  p.name = j.value("name", "");
  p.path = j.value("path", "");
  // `songs` and `setlists` are intentionally not populated: the struct holds
  // non-owning pointers into manager-owned storage. Callers must load the
  // backing objects via SongsManager::loadFromJson / SetlistManager::loadFromJson
  // and then attach pointers to those managers' entries if a full view is
  // needed.
  return p;
}
