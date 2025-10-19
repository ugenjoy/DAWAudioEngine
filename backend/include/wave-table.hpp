#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <vector>

/**
 * @file wave-table.hpp
 * @brief Optimized wavetable oscillator for real-time audio synthesis
 */

/**
 * @class WaveTable
 * @brief Pre-computed wavetable for efficient waveform generation
 * 
 * This class stores pre-computed waveform samples in a lookup table,
 * allowing for efficient real-time audio synthesis without expensive
 * trigonometric calculations. Supports multiple waveform types and
 * provides both fast and interpolated lookup methods.
 * 
 * @note The table is computed once during construction
 * @note Thread-safe for reading after construction
 */
class WaveTable {
 public:
  /**
   * @enum WaveType
   * @brief Available waveform types
   */
  enum WaveType { 
    SINE,     /**< Sine wave */
    SQUARE,   /**< Square wave */
    SAW,      /**< Sawtooth wave */
    TRIANGLE  /**< Triangle wave */
  };

  /**
   * @brief Construct a new WaveTable
   * @param type The waveform type to generate
   * @param tableSize Number of samples in the lookup table (default: 2048)
   * 
   * Larger table sizes provide better quality but use more memory.
   * A size of 2048 is typically sufficient for most applications.
   */
  WaveTable(WaveType type = SINE, int tableSize = 2048) : size(tableSize) {
    table.resize(size);
    float pi = juce::MathConstants<float>::pi;

    for (int i = 0; i < size; ++i) {
      float phase = (2.0f * pi * i) / (float)size;

      switch (type) {
        case SINE:
          table[i] = std::sin(phase);
          break;
        case SQUARE:
          table[i] = (phase < pi) ? 1.0f : -1.0f;
          break;
        case SAW:
          table[i] = 2.0f * (phase / (2.0f * pi)) - 1.0f;
          break;
        case TRIANGLE:
          if (phase < pi)
            table[i] = -1.0f + (2.0f * phase / pi);
          else
            table[i] = 3.0f - (2.0f * phase / pi);
          break;
      }
    }
  }

  /**
   * @brief Get an interpolated sample from the wavetable
   * @param phase The phase angle in radians (0 to 2π)
   * @return Interpolated sample value (typically in range [-1.0, 1.0])
   * 
   * This method uses linear interpolation between adjacent samples
   * for better audio quality. Phase wrapping is handled automatically.
   */
  float getSample(float phase) const {
    float index =
        (phase / (2.0f * juce::MathConstants<float>::pi)) * (float)size;

    while (index >= size)
      index -= size;
    while (index < 0)
      index += size;

    int index0 = (int)index;
    int index1 = (index0 + 1) % size;
    float frac = index - (float)index0;

    return table[index0] + frac * (table[index1] - table[index0]);
  }

  /**
   * @brief Get a sample from the wavetable without interpolation
   * @param phase The phase angle in radians (0 to 2π)
   * @return Sample value (typically in range [-1.0, 1.0])
   * 
   * This method is faster than getSample() but may introduce aliasing.
   * Use for performance-critical code where quality can be sacrificed.
   * Phase wrapping is handled automatically.
   */
  float getSampleFast(float phase) const {
    float index =
        (phase / (2.0f * juce::MathConstants<float>::pi)) * (float)size;
    while (index >= size)
      index -= size;
    while (index < 0)
      index += size;
    return table[(int)index];
  }

 private:
  /** @brief Pre-computed waveform samples */
  std::vector<float> table;
  
  /** @brief Number of samples in the table */
  int size;
};