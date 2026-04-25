// backend/include/services/live-setlist-manager.hpp
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "model/setlist.hpp"
#include "model/song.hpp"

class AppContext;

/**
 * Manages the currently active setlist during a live performance.
 * Tracks position, handles song transitions, and coordinates preloading.
 * Owns a snapshot of the setlist to decouple from editor changes at runtime.
 */
class LiveSetlistManager {
 public:
  LiveSetlistManager() = default;

  /** Load a setlist for live performance. Broadcasts setlist.loaded. */
  void load(const Setlist& setlist,
            const std::vector<Song*>& songs,
            AppContext& ctx);

  /** Load a single song as an implicit setlist of 1. */
  void loadSingle(Song* song, AppContext& ctx);

  /** Unload the active setlist. Broadcasts setlist.unloaded. */
  void unload(AppContext& ctx);

  /** Advance to next song. No-op if already last. Returns success. */
  bool advance(AppContext& ctx);

  /** Go back to previous song. No-op if already first. Returns success. */
  bool previous(AppContext& ctx);

  /** Jump to a specific index. Returns success. */
  bool goTo(int index, AppContext& ctx);

  bool isActive() const { return active; }
  int getCurrentIndex() const { return currentIndex; }
  const Setlist& getSetlist() const { return setlistSnapshot; }

  /**
   * Called by audio engine when playhead reaches endPosition.
   * Handles automatic transitions.
   */
  void onSongEndReached(AppContext& ctx);

 private:
  void changeSong(int newIndex, AppContext& ctx, bool fireEvents = true);
  Song* getSongAt(int index, AppContext& ctx) const;
  nlohmann::json currentSongJson(AppContext& ctx) const;

  bool active = false;
  int currentIndex = 0;
  Setlist setlistSnapshot;
  std::vector<std::string> songIds;  // Ordered IDs for the runtime
};
