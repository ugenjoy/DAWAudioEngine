#include "services/loop-manager.hpp"

#include <algorithm>

void LoopManager::setLoops(const std::vector<Loop>& loops) {
  std::lock_guard<std::mutex> lock(mutex_);
  loops_ = loops;
}

void LoopManager::reset() {
  std::lock_guard<std::mutex> lock(mutex_);
  activeId_.clear();
  enteredIds_.clear();
  cancelledIds_.clear();
  lastPos_.store(0.0, std::memory_order_relaxed);
}

LoopManager::PositionResult LoopManager::checkPosition(double prevPos,
                                                        double newPos) {
  std::lock_guard<std::mutex> lock(mutex_);
  lastPos_.store(newPos, std::memory_order_relaxed);

  PositionResult result;

  // 1. Always detect new loop entries (even when a loop is already active),
  //    so overlapping loops are tracked for activation after the current one ends.
  //    Also handles playback starting inside a loop (newPos already past loop.start).
  for (const auto& loop : loops_) {
    if (!enteredIds_.count(loop.id) && newPos >= loop.start && newPos < loop.end) {
      enteredIds_.insert(loop.id);
    }
  }

  // 2. If a loop is active, check for loop-back.
  if (!activeId_.empty()) {
    auto it = std::find_if(loops_.begin(), loops_.end(),
                           [&](const Loop& l) { return l.id == activeId_; });
    if (it != loops_.end() && newPos >= it->end) {
      result.jumpTo = it->start;
    }
    return result;
  }

  // 3. No active loop — promote the first entered, non-cancelled loop
  //    whose range contains the current position.
  for (const auto& loop : loops_) {
    if (!enteredIds_.count(loop.id)) continue;
    if (cancelledIds_.count(loop.id)) continue;
    if (newPos >= loop.start && newPos < loop.end) {
      activeId_ = loop.id;
      result.activated = true;
      return result;
    }
  }

  return result;
}

bool LoopManager::cancelActiveLoop() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (activeId_.empty()) return false;
  cancelledIds_.insert(activeId_);
  activeId_.clear();
  return true;
}

std::optional<double> LoopManager::exitActiveLoop() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (activeId_.empty()) return std::nullopt;

  auto it = std::find_if(loops_.begin(), loops_.end(),
                         [&](const Loop& l) { return l.id == activeId_; });
  if (it == loops_.end()) {
    cancelledIds_.insert(activeId_);
    activeId_.clear();
    return std::nullopt;
  }

  double endPos = it->end;
  cancelledIds_.insert(activeId_);
  activeId_.clear();
  return endPos;
}

bool LoopManager::hasActiveLoop() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return !activeId_.empty();
}

std::optional<Loop> LoopManager::getActiveLoop() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (activeId_.empty()) return std::nullopt;
  auto it = std::find_if(loops_.begin(), loops_.end(),
                         [&](const Loop& l) { return l.id == activeId_; });
  if (it == loops_.end()) return std::nullopt;
  return *it;
}
