#pragma once
#include "audio-track.hpp"
#include "wave-table.hpp"

/**
 * @file beat-track.hpp
 * @brief Beat-synchronized audio track with ADSR envelope
 */

/**
 * @class BeatTrack
 * @brief Audio track that generates beat-synchronized tones with ADSR envelope
 * 
 * This class generates periodic audio tones synchronized to the global tempo.
 * Each beat uses a wavetable oscillator and applies an ADSR envelope for
 * dynamic sound shaping.
 * 
 * @note Uses a shared static WaveTable for efficiency
 */
class BeatTrack : public AudioTrack {
 public:
  /**
   * @brief Construct a new BeatTrack
   * @param frequency The frequency of the generated tone in Hz
   */
  BeatTrack(float frequency);
  
  /**
   * @brief Destructor
   */
  ~BeatTrack() override;

  /**
   * @brief Generate an audio sample at a given time
   * @param sampleTime The time position in seconds
   * @return The audio sample value with ADSR envelope applied
   */
  float getSampleValue(double sampleTime) override;

  /**
   * @brief Set the mute state of the track
   * @param mute True to mute, false to unmute
   */
  void setMute(bool mute);
  
  /**
   * @brief Set the volume level of the track
   * @param volume Volume level (clamped to range [0.0, 1.0])
   */
  void setVolume(float volume);

 private:
  /**
   * @brief Compute the ADSR envelope value
   * @param timeSinceLastBeat Time elapsed since the last beat started (in seconds)
   * @return Envelope amplitude multiplier (0.0 to 1.0)
   */
  float computeEnveloppe(float timeSinceLastBeat);

  /** @brief Beat interval in seconds (calculated from tempo) */
  float interval;
  
  /** @brief Oscillator frequency in Hz */
  float frequency;
  
  /** @brief Note duration in seconds (before release phase) */
  float duration;

  /** @brief Attack time in seconds */
  float att;
  
  /** @brief Decay time in seconds */
  float dec;
  
  /** @brief Sustain level (0.0 to 1.0) */
  float sus;
  
  /** @brief Release time in seconds */
  float rel;

  /** @brief Shared wavetable oscillator (sine wave, 2048 samples) */
  static WaveTable waveTable;
};