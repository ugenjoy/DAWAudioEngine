#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

/**
 * Manages MIDI output devices using JUCE MidiOutput.
 * Provides lazy device opening, caching, and message sending.
 */
class MidiOutputManager {
 public:
  MidiOutputManager() = default;
  ~MidiOutputManager() = default;

  /**
   * Returns a JSON array of available MIDI output devices.
   * Each entry has "name" and "identifier" fields.
   */
  nlohmann::json getAvailableOutputs() const;

  /**
   * Send a MIDI message to the specified device.
   * @param deviceIdentifier The device identifier (from getAvailableOutputs)
   * @param message The MIDI message to send
   * @return true if sent successfully, false if device not found or failed to open
   */
  bool sendMessage(const std::string& deviceIdentifier,
                   const juce::MidiMessage& message);

  /**
   * Close all open device connections.
   */
  void closeAll();

 private:
  /**
   * Get or open a MIDI output device by identifier.
   * Caches open devices for reuse.
   */
  juce::MidiOutput* getOrOpenDevice(const std::string& deviceIdentifier);

  std::unordered_map<std::string, std::unique_ptr<juce::MidiOutput>>
      openDevices;
};
