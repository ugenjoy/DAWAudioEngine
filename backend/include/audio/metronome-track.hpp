#pragma once
#include "audio/audio-track.hpp"
#include "audio/dsp/wave-table.hpp"

/**
 * Audio track that generates a beat-synchronized click using a wavetable
 * oscillator and ADSR envelope. Each song has one dedicated MetronomeTrack.
 */
class MetronomeTrack : public AudioTrack {
 public:
  explicit MetronomeTrack(float frequency = 1000.0f);
  ~MetronomeTrack() override;

  float getSampleValue(double sampleTime, float tempo) override;

  void renderBlock(juce::AudioBuffer<float>& buffer,
                   int startSample,
                   int numSamples,
                   double startTime,
                   float tempo) override;

  // Serialization
  nlohmann::json toJson() const override;
  static std::unique_ptr<MetronomeTrack> fromJson(const nlohmann::json& j);
  std::string getTrackType() const override { return "MetronomeTrack"; }

  bool canFreeze() const override { return true; }
  void freeze(float tempo, double sampleRate) override;

 private:
  float computeEnvelope(float timeSinceLastBeat) const;

  float interval;
  float frequency;
  float duration;
  float att;
  float dec;
  float sus;
  float rel;

  static WaveTable waveTable;
};
