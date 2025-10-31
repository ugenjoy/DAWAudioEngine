#pragma once

#include <atomic>

// TODO: [LOW] Add configuration file support (JSON/XML) for loading/saving
// settings
// TODO: [MEDIUM] Add tempo change ramping to avoid clicks
// TODO: [LOW] Add time signature change callback system
// TODO: [LOW] Add preset management system

/**
 * @file audio-context.hpp
 * @brief Global audio context singleton providing shared audio configuration
 */

/**
 * @class AudioContext
 * @brief Singleton class managing global audio parameters
 *
 * This class provides a thread-safe singleton instance that stores global
 * audio configuration parameters such as sample rate, buffer size, tempo,
 * and time signature. These parameters are accessible from any part of the
 * audio engine.
 *
 * @note This class uses the Meyer's Singleton pattern for thread-safety
 * @note Tempo and time signature use atomic types for thread-safe access
 */
class AudioContext {
 public:
  /**
   * @brief Get the singleton instance of AudioContext
   * @return Reference to the unique AudioContext instance
   * @note Thread-safe initialization (C++11 and later)
   */
  static AudioContext& getInstance() {
    static AudioContext instance;
    return instance;
  }

  /** @brief Current sample rate in Hz (default: 44100.0) */
  double sampleRate = 44100.0;

  /** @brief Audio buffer size in samples (default: 512) */
  int bufferSize = 512;

  /** @brief Current tempo in beats per minute (thread-safe, default: 120.0) */
  std::atomic<float> tempoBPM{120.0f};
  // TODO: [MEDIUM] Add tempo ramping to avoid clicks on tempo change:
  // std::atomic<float> targetTempoBPM{120.0f};
  // float tempoRampTime = 0.1f;  // 100ms ramp

  /** @brief Time signature numerator (thread-safe, default: 4) */
  std::atomic<int> timeSignatureNumerator{4};

  /** @brief Time signature denominator (thread-safe, default: 4) */
  std::atomic<int> timeSignatureDenominator{4};

  /**
   * @brief Deleted copy constructor to prevent copying
   * @note Singleton pattern - only one instance allowed
   */
  AudioContext(const AudioContext&) = delete;

  /**
   * @brief Deleted assignment operator to prevent assignment
   * @note Singleton pattern - only one instance allowed
   */
  AudioContext& operator=(const AudioContext&) = delete;

 private:
  /**
   * @brief Private default constructor
   * @note Only accessible through getInstance()
   */
  AudioContext() = default;
};