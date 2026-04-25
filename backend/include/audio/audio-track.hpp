#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <atomic>
#include <cmath>
#include <nlohmann/json.hpp>
#include <string>

/**
 * Abstract base class for all audio track types.
 *
 * Derived classes must implement getSampleValue() and renderBlock()
 * to generate audio. All public state uses atomics or is only accessed
 * from the command thread, so the audio thread can read safely.
 */
class AudioTrack {
 public:
  AudioTrack();
  virtual ~AudioTrack() = default;

  virtual float getSampleValue(double sampleTime, float tempo) = 0;

  /**
   * Render a block of audio samples into a pre-allocated buffer.
   * Preferred over per-sample getSampleValue() for batch processing
   * and reduced virtual call overhead.
   */
  virtual void renderBlock(juce::AudioBuffer<float>& buffer,
                           int startSample,
                           int numSamples,
                           double startTime,
                           float tempo) = 0;

  virtual void setMute(bool shouldMute);
  virtual void setSolo(bool shouldSolo);
  virtual void setVolume(float volumeDb);

  /** Convert volume (dB) to linear gain. -80 dB maps to 0. */
  float getLinearGain() const {
    return volume <= -80.0f ? 0.0f : std::pow(10.0f, volume / 20.0f);
  }

  virtual nlohmann::json toJson() const = 0;
  virtual std::string getTrackType() const = 0;

  virtual void sampleRateChanged() {};

  /**
   * Whether this track type supports freezing (pre-rendering a period).
   * MetronomeTrack returns true; AudioFileTrack returns false (position-dependent).
   */
  virtual bool canFreeze() const { return false; }

  /**
   * Pre-render one period into the frozen buffer.
   * Called on the command thread. Sets frozen = true when done.
   */
  virtual void freeze(float tempo, double sampleRate) {}

  void unfreeze();

  std::atomic<bool> frozen{false};
  juce::AudioBuffer<float> frozenBuffer;
  int frozenPeriodSamples{0};

  // Input routing
  void setInputChannel(int channel);
  int getInputChannel() const;
  void setInputStereo(bool stereo);
  bool isInputStereo() const;
  void setMonitoring(bool enabled);
  bool isMonitoring() const;

  std::string getId() const { return id; }

  std::string id;
  std::string name;
  float volume;
  float pan;
  bool mute;
  bool solo;
  int color;  // palette index (1-8)

  std::atomic<float> peakLevel{0.0f};

  std::atomic<int> inputChannel{-1};
  std::atomic<bool> inputStereo{false};
  std::atomic<bool> monitoring{false};
};
