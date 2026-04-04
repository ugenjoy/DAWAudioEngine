#include "services/song-preloader.hpp"

#include <juce_core/juce_core.h>

#include "model/song.hpp"

SongPreloader::~SongPreloader() { cancelPending(); }

void SongPreloader::onSongChanged(Song* current,
                                  const std::vector<Song*>& songs,
                                  const std::string& audioDir) {
  preloadNext(current, songs, audioDir);
}

void SongPreloader::onLiveModeEntered(Song* current,
                                      const std::vector<Song*>& songs,
                                      const std::string& audioDir) {
  preloadNext(current, songs, audioDir);
}

void SongPreloader::onLiveModeExited() {
  cancelPending();

  // Unload the preloaded song if any
  std::lock_guard<std::mutex> lock(mutex);
  if (preloadedSong != nullptr &&
      preloadedSong->getLoadState() == SongLoadState::Loaded) {
    juce::Logger::writeToLog("[SongPreloader] Unloading preloaded song: " +
                             juce::String(preloadedSong->getName()));
    preloadedSong->unloadAudio();
  }
  preloadedSong = nullptr;
}

void SongPreloader::cancelPending() {
  std::lock_guard<std::mutex> lock(mutex);
  cancelPendingLocked();
}

Song* SongPreloader::getPreloadedSong() {
  std::lock_guard<std::mutex> lock(mutex);
  return preloadedSong;
}

void SongPreloader::cancelPendingLocked() {
  if (pendingFuture.valid()) {
    pendingFuture.get();
  }
}

void SongPreloader::preloadNext(Song* current,
                                const std::vector<Song*>& songs,
                                const std::string& audioDir) {
  std::lock_guard<std::mutex> lock(mutex);

  cancelPendingLocked();  // Wait for any in-progress preload

  // Unload old preloaded song if it is no longer needed (not the new current)
  if (preloadedSong && preloadedSong != current &&
      preloadedSong->getLoadState() == SongLoadState::Loaded) {
    juce::Logger::writeToLog("[SongPreloader] Unloading stale preloaded song: " +
                             juce::String(preloadedSong->getName()));
    preloadedSong->unloadAudio();
  }
  preloadedSong = nullptr;

  // Find current song index
  int currentIndex = -1;
  for (int i = 0; i < static_cast<int>(songs.size()); ++i) {
    if (songs[i] == current) {
      currentIndex = i;
      break;
    }
  }

  if (currentIndex < 0) return;

  // Check if there is a next song
  int nextIndex = currentIndex + 1;
  if (nextIndex >= static_cast<int>(songs.size())) {
    juce::Logger::writeToLog("[SongPreloader] No next song to preload (last in list)");
    return;
  }

  Song* nextSong = songs[nextIndex];

  // Skip if already loaded
  if (nextSong->getLoadState() == SongLoadState::Loaded) {
    juce::Logger::writeToLog("[SongPreloader] Next song already loaded: " +
                             juce::String(nextSong->getName()));
    preloadedSong = nextSong;
    return;
  }

  // Launch async preload — capture only the Song* and audioDir, not the vector
  preloadedSong = nextSong;
  std::string dir = audioDir;

  juce::Logger::writeToLog("[SongPreloader] Preloading: " +
                           juce::String(nextSong->getName()));

  pendingFuture = std::async(std::launch::async, [nextSong, dir]() {
    nextSong->loadAudio(dir);
    juce::Logger::writeToLog("[SongPreloader] Preload complete: " +
                             juce::String(nextSong->getName()));
  });
}
