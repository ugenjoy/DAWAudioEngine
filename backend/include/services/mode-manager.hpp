#pragma once

#include <atomic>
#include <functional>

// Prevent copy/assign manually since we don't want to pull in all of JUCE here

enum class AppMode { Live,
                     Edit };

/**
 * Manages the application mode (Live vs Edit).
 *
 * Live mode: only performance actions are allowed (play, stop, navigate).
 * Edit mode: full DAW editing capabilities are available.
 *
 * The backend is the source of truth for the current mode.
 * Thread-safe via std::atomic.
 */
class ModeManager {
 public:
  ModeManager() = default;

  AppMode getMode() const { return currentMode.load(); }
  bool isLiveMode() const { return currentMode.load() == AppMode::Live; }
  bool isEditMode() const { return currentMode.load() == AppMode::Edit; }

  /**
   * Attempt to switch to the given mode.
   * Switching to Edit is rejected if playback is currently active.
   * @return true if mode changed, false if rejected
   */
  bool setMode(AppMode mode);

  /**
   * Provide a callable that returns the current playback state.
   * Used to prevent switching to Edit while playing.
   */
  void setPlayingStateProvider(std::function<bool()> provider);

  ModeManager(const ModeManager&) = delete;
  ModeManager& operator=(const ModeManager&) = delete;

 private:
  std::atomic<AppMode> currentMode{AppMode::Live};
  std::function<bool()> isPlaying;
};
