#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstdint>
#include <vector>

namespace midi {

/**
 * Build a juce::MidiMessage from raw bytes (1-3 bytes).
 */
inline juce::MidiMessage fromBytes(const std::vector<uint8_t>& bytes) {
  if (bytes.size() == 1) {
    return juce::MidiMessage(bytes[0]);
  } else if (bytes.size() == 2) {
    return juce::MidiMessage(bytes[0], bytes[1]);
  } else {
    return juce::MidiMessage(bytes[0], bytes[1], bytes[2]);
  }
}

}  // namespace midi
