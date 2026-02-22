#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <string>

/**
 * @file audio-track.hpp
 * @brief Abstract base class for all audio track types
 */

/**
 * @class AudioTrack
 * @brief Abstract base class representing a single audio track
 *
 * This class defines the interface for all audio track types in the DAW.
 * Derived classes must implement the getSampleValue() method to generate
 * audio samples at a given time.
 *
 * @note This is an abstract class and cannot be instantiated directly
 */
class AudioTrack {
 public:
  /**
   * @brief Default constructor
   * Initializes volume to 0.4, pan to center (0.0), and mute to false
   */
  AudioTrack();

  /**
   * @brief Virtual destructor
   * Ensures proper cleanup of derived classes
   */
  virtual ~AudioTrack() = default;

  /**
   * @brief Generate an audio sample at a given time
   * @param sampleTime The time position in seconds
   * @param tempo The tempo in beats per minute (from parent Song)
   * @return The audio sample value (typically in range [-1.0, 1.0])
   * @note Pure virtual function - must be implemented by derived classes
   */
  virtual float getSampleValue(double sampleTime, float tempo) = 0;

  /**
   * @brief Render a block of audio samples (batch processing)
   * @param buffer The audio buffer to fill (mono, single channel)
   * @param startSample The starting sample index in the buffer
   * @param numSamples The number of samples to render
   * @param startTime The time position in seconds for the first sample
   * @param tempo The tempo in beats per minute (from parent Song)
   *
   * This method provides optimized batch processing instead of per-sample
   * rendering. It allows for SIMD optimizations and reduces virtual call
   * overhead.
   *
   * @note Pure virtual function - must be implemented by derived classes
   * @note Buffer should be pre-allocated with sufficient size
   */
  virtual void renderBlock(juce::AudioBuffer<float>& buffer,
                           int startSample,
                           int numSamples,
                           double startTime,
                           float tempo) = 0;

  /**
   * @brief Set the mute state of the track
   * @param mute True to mute, false to unmute
   */
  virtual void setMute(bool shouldMute);

  /**
   * @brief Set the solo state of the track
   * @param solo True to solo, false to unsolo
   */
  virtual void setSolo(bool shouldSolo);

  /**
   * @brief Set the volume level of the track
   * @param volume Volume level (clamped to range [0.0, 1.0])
   */
  virtual void setVolume(float volume);

  /**
   * @brief Serialize track to JSON
   * @return JSON representation of the track
   * @note Pure virtual function - must be implemented by derived classes
   */
  virtual nlohmann::json toJson() const = 0;

  /**
   * @brief Get the track type identifier
   * @return String identifier for the track type (e.g., "MetronomeTrack")
   * @note Pure virtual function - must be implemented by derived classes
   */
  virtual std::string getTrackType() const = 0;

  /** @brief Called when sample rate changes to allow resampling */
  virtual void sampleRateChanged() {};

  /**
   * @brief Whether this track type supports freezing (pre-rendering a period).
   * MetronomeTrack returns true; AudioFileTrack returns false (position-dependent).
   */
  virtual bool canFreeze() const { return false; }

  /**
   * @brief Pre-render one period into the frozen buffer.
   * Called on the command thread. Sets frozen = true when done.
   * @param tempo Current tempo in BPM
   * @param sampleRate Current sample rate
   */
  virtual void freeze(float tempo, double sampleRate) {}

  /** @brief Release the frozen buffer and resume live rendering. */
  void unfreeze();

  /** @brief Whether this track is using the frozen buffer. */
  std::atomic<bool> frozen{false};

  /** @brief Pre-rendered audio buffer (stereo, one period). */
  juce::AudioBuffer<float> frozenBuffer;

  /** @brief Number of valid samples in frozenBuffer. */
  int frozenPeriodSamples{0};

  // Input routing
  void setInputChannel(int channel);
  int getInputChannel() const;
  void setInputStereo(bool stereo);
  bool isInputStereo() const;
  void setMonitoring(bool enabled);
  bool isMonitoring() const;

  // Getters
  std::string getId() const { return id; }

  /** @brief Unique identifier for this track */
  std::string id;

  /** @brief Track name */
  std::string name;

  /** @brief Track volume level (0.0 to 1.0) */
  float volume;

  /** @brief Pan position (-1.0 = left, 0.0 = center, 1.0 = right) */
  float pan;

  /** @brief Mute state (true = muted, false = playing) */
  bool mute;

  /** @brief Solo state (true = soloed) */
  bool solo;

  /** @brief Selected input channel index (-1 = no input) */
  std::atomic<int> inputChannel{-1};

  /** @brief Input stereo mode (false = mono duplicated, true = stereo pair) */
  std::atomic<bool> inputStereo{false};

  /** @brief Monitoring state (true = input audio replaces clips) */
  std::atomic<bool> monitoring{false};
};