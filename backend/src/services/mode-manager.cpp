#include "services/mode-manager.hpp"

#include "services/session.hpp"

void ModeManager::setSession(Session* s) {
  session_ = s;
}

AppMode ModeManager::getMode() const {
  if (session_) {
    if (session_->isEdit()) return AppMode::Edit;
    if (session_->isLive()) return AppMode::Live;
  }
  return currentMode.load();
}

bool ModeManager::isLiveMode() const {
  if (session_) return session_->isLive();
  return currentMode.load() == AppMode::Live;
}

bool ModeManager::isEditMode() const {
  if (session_) return session_->isEdit();
  return currentMode.load() == AppMode::Edit;
}

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
