#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <nlohmann/json.hpp>
#include "audio-track.hpp"
#include "song.hpp"

class SongsManager {
 public:
  SongsManager();
  ~SongsManager();

  void addSong(std::unique_ptr<Song>);
  void removeSong();
  std::vector<Song*> getSongList();
  Song* getSong(int songId);

  // Serialization
  nlohmann::json toJson() const;
  void loadFromJson(const nlohmann::json& j);

 private:
  std::vector<std::unique_ptr<Song>> songs;
};