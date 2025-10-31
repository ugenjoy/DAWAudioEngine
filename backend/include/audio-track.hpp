#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

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
   * @return The audio sample value (typically in range [-1.0, 1.0])
   * @note Pure virtual function - must be implemented by derived classes
   */
  virtual float getSampleValue(double sampleTime) = 0;

  /**
   * @brief Render a block of audio samples (batch processing)
   * @param buffer The audio buffer to fill (mono, single channel)
   * @param startSample The starting sample index in the buffer
   * @param numSamples The number of samples to render
   * @param startTime The time position in seconds for the first sample
   *
   * This method provides optimized batch processing instead of per-sample
   * rendering. It allows for SIMD optimizations and reduces virtual call
   * overhead.
   *
   * @note Pure virtual function - must be implemented by derived classes
   * @note Buffer should be pre-allocated with sufficient size
   */
  virtual void renderBlock(juce::AudioBuffer<float>& buffer, int startSample,
                          int numSamples, double startTime) = 0;

  /**
   * @brief Set the mute state of the track
   * @param mute True to mute, false to unmute
   */
  virtual void setMute(bool shouldMute);

  /**
   * @brief Set the volume level of the track
   * @param volume Volume level (clamped to range [0.0, 1.0])
   */
  virtual void setVolume(float volume);

  /** @brief Track volume level (0.0 to 1.0) */
  float volume;

  /** @brief Pan position (-1.0 = left, 0.0 = center, 1.0 = right) */
  float pan;

  /** @brief Mute state (true = muted, false = playing) */
  bool mute;
};