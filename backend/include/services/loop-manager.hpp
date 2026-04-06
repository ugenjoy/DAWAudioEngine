#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "model/loop.hpp"

/**
 * Manages runtime loop state during playback.
 *
 * Thread safety:
 * - checkPosition() is called from the audio thread.
 * - All other methods are called from the message thread.
 * - A mutex protects shared state.
 */
class LoopManager {
 public:
  struct PositionResult {
    std::optional<double> jumpTo;  // set → loop-back to this position
    bool activated = false;        // a new loop just became active
  };

  void setLoops(const std::vector<Loop>& loops);
  void reset();

  // Audio thread: call after advancing playhead. prevPos = before, newPos = after.
  PositionResult checkPosition(double prevPos, double newPos);

  // Message thread: deactivate current loop. Returns true if there was one.
  bool cancelActiveLoop();

  // Message thread: returns end position to jump to, or nullopt.
  std::optional<double> exitActiveLoop();

  bool hasActiveLoop() const;
  std::optional<Loop> getActiveLoop() const;

 private:
  mutable std::mutex mutex_;
  std::vector<Loop> loops_;
  std::string activeId_;
  std::set<std::string> enteredIds_;   // loops whose start has been crossed
  std::set<std::string> cancelledIds_; // cancelled/exited loops (won't re-activate)
  std::atomic<double> lastPos_{0.0};
};
