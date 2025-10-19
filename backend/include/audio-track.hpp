#pragma once

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

 protected:
  /** @brief Track volume level (0.0 to 1.0) */
  float volume;
  
  /** @brief Pan position (-1.0 = left, 0.0 = center, 1.0 = right) */
  float pan;
  
  /** @brief Mute state (true = muted, false = playing) */
  bool mute;
};