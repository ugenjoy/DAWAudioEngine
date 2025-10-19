#pragma once
#include <atomic>

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