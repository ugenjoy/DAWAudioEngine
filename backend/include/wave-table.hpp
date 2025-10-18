#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <vector>

class WaveTable {
 public:
  enum WaveType { SINE, SQUARE, SAW, TRIANGLE };

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
  std::vector<float> table;
  int size;
};