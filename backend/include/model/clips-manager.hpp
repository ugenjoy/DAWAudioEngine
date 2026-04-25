#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <nlohmann/json.hpp>

#include "audio/audio-clip.hpp"

class ClipsManager {
 public:
  ClipsManager();
  ~ClipsManager();

  void addClip(std::unique_ptr<AudioClip>);
  bool removeClip(const std::string& clipId);
  bool moveClip(const std::string& clipId, double newPosition);
  void getClipList();

  void renderClips(juce::AudioBuffer<float>& clipBuffer, int numSamples,
                   double currentPosition);
  void sampleRateChanged();

  void loadAudio(const std::string& audioDir);
  void unloadAudio();

  // Serialization
  nlohmann::json toJson() const;
  void loadFromJson(const nlohmann::json& j, const std::string& audioDir,
                    bool loadAudio = true);

  // Getter for clips
  const std::vector<std::unique_ptr<AudioClip>>& getClips() const {
    return clips;
  }

 private:
  std::vector<std::unique_ptr<AudioClip>> clips;
  std::string audioDir;
};