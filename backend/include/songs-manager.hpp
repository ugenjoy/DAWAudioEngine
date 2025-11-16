#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
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

 private:
  std::vector<std::unique_ptr<Song>> songs;
};