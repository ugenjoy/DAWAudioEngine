#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <nlohmann/json.hpp>

#include "audio/audio-clip.hpp"

class ClipsManager {
 public:
  ClipsManager();
  ~ClipsManager();

  void addClip(std::unique_ptr<AudioClip>);
  void removeClip();
  void getClipList();

  void renderClips(juce::AudioBuffer<float>& clipBuffer, int numSamples,
                   double currentPosition);

  // Serialization
  nlohmann::json toJson() const;
  void loadFromJson(const nlohmann::json& j);

  // Getter for clips
  const std::vector<std::unique_ptr<AudioClip>>& getClips() const {
    return clips;
  }

 private:
  std::vector<std::unique_ptr<AudioClip>> clips;
};