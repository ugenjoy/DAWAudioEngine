#include "model/clips-manager.hpp"

#include "audio/audio-clip.hpp"

ClipsManager::ClipsManager() = default;
ClipsManager::~ClipsManager() = default;

void ClipsManager::addClip(std::unique_ptr<AudioClip> clip) {
  clips.push_back(std::move(clip));
}

void ClipsManager::removeClip() {
  // To implement
}

void ClipsManager::renderClips(juce::AudioBuffer<float>& clipBuffer,
                               int numSamples, double currentPosition) {
  for (size_t clipIdx = 0; clipIdx < clips.size(); ++clipIdx) {
    clips[clipIdx]->renderBlock(clipBuffer, 0, numSamples, currentPosition);
  }
}

void ClipsManager::sampleRateChanged() {
  for (auto& clip : clips) {
    clip->loadAudioFile();
  }
}

nlohmann::json ClipsManager::toJson() const {
  nlohmann::json j = nlohmann::json::array();

  for (const auto& clip : clips) {
    j.push_back(clip->toJson());
  }

  return j;
}

void ClipsManager::loadFromJson(const nlohmann::json& j) {
  clips.clear();

  if (!j.is_array()) {
    return;
  }

  for (const auto& clipJson : j) {
    clips.push_back(AudioClip::fromJson(clipJson));
  }
}