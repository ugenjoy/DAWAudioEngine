#include "services/mode-manager.hpp"

void ModeManager::setPlayingStateProvider(std::function<bool()> provider) {
  isPlaying = std::move(provider);
}

bool ModeManager::setMode(AppMode mode) {
  if (mode == AppMode::Edit && isPlaying && isPlaying()) {
    return false;  // Cannot enter Edit mode while playing
  }
  currentMode.store(mode);
  return true;
}
