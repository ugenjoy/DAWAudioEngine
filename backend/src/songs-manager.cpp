#include "songs-manager.hpp"

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
    songs.push_back(Song::fromJson(songJson));
  }
}