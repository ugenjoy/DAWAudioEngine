#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

#include "audio/audio-track.hpp"
#include "audio/dsp/wave-table.hpp"
#include "model/clips-manager.hpp"

class AudioFileTrack : public AudioTrack {
 public:
  explicit AudioFileTrack();

  ~AudioFileTrack() override;

  float getSampleValue(double sampleTime, float tempo) override;

  void renderBlock(juce::AudioBuffer<float>& buffer, int startSample,
                   int numSamples, double startTime, float tempo) override;

  // Serialization
  nlohmann::json toJson() const override;
  static std::unique_ptr<AudioFileTrack> fromJson(const nlohmann::json& j);

  std::string getTrackType() const override { return "AudioFileTrack"; }

 private:
  std::unique_ptr<ClipsManager> clipsManager;
};