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

  void loadAudio(const std::string& audioDir);
  void unloadAudio();

  // Serialization
  nlohmann::json toJson() const override;
  static std::unique_ptr<AudioFileTrack> fromJson(const nlohmann::json& j,
                                                   const std::string& audioDir,
                                                   bool loadAudio = true);

  std::string getTrackType() const override { return "AudioFileTrack"; }
  void sampleRateChanged() override;

  ClipsManager* getClipsManager() const { return clipsManager.get(); }

 private:
  std::unique_ptr<ClipsManager> clipsManager;
};