#include "services/songs-manager.hpp"

#include <algorithm>

SongsManager::SongsManager() = default;
SongsManager::~SongsManager() = default;

void SongsManager::addSong(std::unique_ptr<Song> song) {
  songs.push_back(std::move(song));
}

void SongsManager::removeSong() {
  // To implement
}

std::vector<Song*> SongsManager::getSongList() {
  std::vector<Song*> newSongs;

  for (int i = 0; i < songs.size(); i++) {
    newSongs.push_back(songs[i].get());
  }

  return newSongs;
}

Song* SongsManager::getSong(int songId) {
  if (songId < 0 || static_cast<size_t>(songId) >= songs.size()) {
    return nullptr;
  }
  return songs[songId].get();
}

Song* SongsManager::getSongById(const std::string& uuid) {
  for (auto& song : songs) {
    if (song->getId() == uuid) return song.get();
  }
  return nullptr;
}

bool SongsManager::renameSong(const std::string& uuid,
                              const std::string& name) {
  auto* song = getSongById(uuid);
  if (!song) return false;
  song->setName(name);
  return true;
}

bool SongsManager::reorderSong(const std::string& uuid, int newIndex) {
  int oldIndex = -1;
  for (int i = 0; i < static_cast<int>(songs.size()); i++) {
    if (songs[i]->getId() == uuid) {
      oldIndex = i;
      break;
    }
  }
  if (oldIndex < 0) return false;

  auto moved = std::move(songs[oldIndex]);
  songs.erase(songs.begin() + oldIndex);
  int clamped = std::max(0, std::min(static_cast<int>(songs.size()), newIndex));
  songs.insert(songs.begin() + clamped, std::move(moved));
  return true;
}

nlohmann::json SongsManager::toJson() const {
  nlohmann::json j = nlohmann::json::array();

  for (const auto& song : songs) {
    j.push_back(song->toJson());
  }

  return j;
}

void SongsManager::loadFromJson(const nlohmann::json& j) {
  songs.clear();

  if (!j.is_array()) {
    return;
  }

  for (const auto& songJson : j) {
    // Load metadata only — audio is loaded on demand when a song is activated
    songs.push_back(Song::fromJson(songJson, "", false));
  }
}

bool SongsManager::removeProjectEventRule(const std::string& ruleId) {
  auto it = std::find_if(
      projectEventRules.begin(), projectEventRules.end(),
      [&](const EventRule& r) { return r.id == ruleId; });
  if (it == projectEventRules.end()) return false;
  projectEventRules.erase(it);
  return true;
}

bool SongsManager::updateProjectEventRule(const std::string& ruleId,
                                          const EventRule& updated) {
  auto it = std::find_if(
      projectEventRules.begin(), projectEventRules.end(),
      [&](const EventRule& r) { return r.id == ruleId; });
  if (it == projectEventRules.end()) return false;
  *it = updated;
  return true;
}

nlohmann::json SongsManager::projectEventsToJson() const {
  nlohmann::json j = nlohmann::json::array();
  for (const auto& rule : projectEventRules) {
    j.push_back(rule.toJson());
  }
  return j;
}

void SongsManager::loadProjectEventsFromJson(const nlohmann::json& j) {
  projectEventRules.clear();
  if (!j.is_array()) return;
  for (const auto& ruleJson : j) {
    projectEventRules.push_back(EventRule::fromJson(ruleJson));
  }
}