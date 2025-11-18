#pragma once
#include "audio-track.hpp"
#include "wave-table.hpp"

// TODO: [MEDIUM] Add ADSR configuration structure:
// struct ADSRParameters {
//   float attackTime = 0.01f;
//   float decayTime = 0.02f;
//   float sustainLevel = 0.4f;
//   float releaseTime = 0.02f;
//   enum Curve { Linear, Exponential, Logarithmic };
//   Curve attackCurve = Exponential;
//   Curve decayCurve = Exponential;
//   Curve releaseCurve = Exponential;
// };

// TODO: [LOW] Add waveform type selection (currently hardcoded to SINE)
// TODO: [LOW] Add velocity sensitivity for dynamic expression

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
  explicit BeatTrack(float frequency);

  /**
   * @brief Destructor
   */
  ~BeatTrack() override;

  /**
   * @brief Generate an audio sample at a given time
   * @param sampleTime The time position in seconds
   * @param tempo The tempo in beats per minute
   * @return The audio sample value with ADSR envelope applied
   */
  float getSampleValue(double sampleTime, float tempo) override;

  /**
   * @brief Render a block of audio samples (optimized batch processing)
   * @param buffer The audio buffer to fill (mono, single channel)
   * @param startSample The starting sample index in the buffer
   * @param numSamples The number of samples to render
   * @param startTime The time position in seconds for the first sample
   * @param tempo The tempo in beats per minute
   */
  void renderBlock(juce::AudioBuffer<float>& buffer,
                   int startSample,
                   int numSamples,
                   double startTime,
                   float tempo) override;

  // Serialization
  nlohmann::json toJson() const override;
  static std::unique_ptr<BeatTrack> fromJson(const nlohmann::json& j);
  std::string getTrackType() const override { return "BeatTrack"; }

  // TODO: [MEDIUM] Add ADSR configuration methods:
  // void setADSRParameters(const ADSRParameters& params);
  // ADSRParameters getADSRParameters() const;
  // void setAttackTime(float time);
  // void setDecayTime(float time);
  // void setSustainLevel(float level);
  // void setReleaseTime(float time);

  // TODO: [LOW] Add waveform selection:
  // void setWaveform(WaveTable::WaveformType type);
  // WaveTable::WaveformType getWaveform() const;

  // TODO: [LOW] Add velocity control:
  // void setVelocity(float velocity);  // 0.0 to 1.0
  // float getVelocity() const;

 private:
  /**
   * @brief Compute the ADSR envelope value
   * @param timeSinceLastBeat Time elapsed since the last beat started (in
   * seconds)
   * @return Envelope amplitude multiplier (0.0 to 1.0)
   */
  float computeEnveloppe(float timeSinceLastBeat) const;

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

  // TODO: [MEDIUM] Add configurable ADSR parameters member:
  // ADSRParameters adsrParams;

  // TODO: [LOW] Add per-instance waveform (replace static shared wavetable):
  // std::unique_ptr<WaveTable> oscillator;

  // TODO: [LOW] Add velocity member for dynamic response:
  // float velocity = 1.0f;

  // TODO: [LOW] Add volume and mute state members:
  // float volume = 0.4f;
  // bool muted = false;
};