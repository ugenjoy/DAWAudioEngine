#pragma once

#include <future>
#include <mutex>
#include <string>
#include <vector>

class Song;

/**
 * Preloads the next song in background during live mode.
 *
 * Only one preload runs at a time. The preloader never touches the active song
 * in the audio engine — it only loads songs that are not yet in the render path.
 *
 * Memory model: at most 2 songs have loaded audio (active + preloaded next).
 */
class SongPreloader {
 public:
  SongPreloader() = default;
  ~SongPreloader();

  SongPreloader(const SongPreloader&) = delete;
  SongPreloader& operator=(const SongPreloader&) = delete;

  /**
   * Called after a song transition in live mode.
   * Preloads the next song in the list after `current`.
   * Unloading of the previous song is handled by the caller (LoadSongCommand).
   */
  void onSongChanged(Song* current, const std::vector<Song*>& songs,
                     const std::string& audioDir);

  /**
   * Called when entering live mode.
   * Preloads the next song after `current`.
   */
  void onLiveModeEntered(Song* current, const std::vector<Song*>& songs,
                         const std::string& audioDir);

  /**
   * Called when exiting live mode.
   * Cancels any pending preload and unloads the preloaded song.
   */
  void onLiveModeExited();

  /**
   * Cancel any pending preload, blocking until completion.
   */
  void cancelPending();

  /**
   * Returns the currently preloaded song, or nullptr if none.
   * Thread-safe (acquires mutex).
   */
  Song* getPreloadedSong();

 private:
  void preloadNext(Song* current, const std::vector<Song*>& songs,
                   const std::string& audioDir);

  /** Cancel pending preload. Caller must hold mutex. */
  void cancelPendingLocked();

  std::future<void> pendingFuture;
  Song* preloadedSong = nullptr;
  std::mutex mutex;
};
