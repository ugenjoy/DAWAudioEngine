#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>

#include <nlohmann/json.hpp>
#include <string>

class AudioClip {
 public:
  explicit AudioClip();
  ~AudioClip();

  void loadAudioFile();

  void renderBlock(juce::AudioBuffer<float>& buffer, int startSample,
                   int numSamples, double startTime);

  void setGain(float gain);

  std::string getClipType() const { return "AudioClip"; }
  std::string getId() const { return id; }

  nlohmann::json toJson() const;
  static std::unique_ptr<AudioClip> fromJson(const nlohmann::json& j);

 private:
  std::string id;
  std::string name;
  std::string fileName;
  float gain;

  double position;
  double duration;
  double offset;

  juce::AudioFormatManager formatManager;
  juce::AudioBuffer<float> audioData;
  bool loaded = false;
};