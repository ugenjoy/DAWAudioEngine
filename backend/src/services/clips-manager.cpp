#include "model/clips-manager.hpp"

#include "audio/audio-clip.hpp"

ClipsManager::ClipsManager() = default;
ClipsManager::~ClipsManager() = default;

void ClipsManager::addClip(std::unique_ptr<AudioClip> clip) {
  clips.push_back(std::move(clip));
}

bool ClipsManager::removeClip(const std::string& clipId) {
  auto it = std::find_if(clips.begin(), clips.end(),
                         [&](const auto& c) { return c->getId() == clipId; });
  if (it == clips.end()) return false;
  clips.erase(it);
  return true;
}

bool ClipsManager::moveClip(const std::string& clipId, double newPosition) {
  auto it = std::find_if(clips.begin(), clips.end(),
                         [&](const auto& c) { return c->getId() == clipId; });
  if (it == clips.end()) return false;
  (*it)->setPosition(newPosition);
  return true;
}

void ClipsManager::renderClips(juce::AudioBuffer<float>& clipBuffer,
                               int numSamples, double currentPosition) {
  for (size_t clipIdx = 0; clipIdx < clips.size(); ++clipIdx) {
    clips[clipIdx]->renderBlock(clipBuffer, 0, numSamples, currentPosition);
  }
}

void ClipsManager::loadAudio(const std::string& dir) {
  audioDir = dir;
  for (auto& clip : clips) {
    if (!clip->isLoaded() && !clip->getFileName().empty()) {
      clip->loadAudioFile(audioDir);
    }
  }
}

void ClipsManager::unloadAudio() {
  for (auto& clip : clips) {
    clip->unloadAudio();
  }
}

void ClipsManager::sampleRateChanged() {
  for (auto& clip : clips) {
    if (clip->isLoaded() && !audioDir.empty()) {
      clip->loadAudioFile(audioDir);
    }
  }
}

nlohmann::json ClipsManager::toJson() const {
  nlohmann::json j = nlohmann::json::array();

  for (const auto& clip : clips) {
    j.push_back(clip->toJson());
  }

  return j;
}

void ClipsManager::loadFromJson(const nlohmann::json& j,
                                 const std::string& dir,
                                 bool loadAudio) {
  clips.clear();
  audioDir = dir;

  if (!j.is_array()) {
    return;
  }

  for (const auto& clipJson : j) {
    clips.push_back(AudioClip::fromJson(clipJson, audioDir, loadAudio));
  }
}