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
   * Get or open a MIDI output device by name (preferred) or identifier (legacy).
   * Resolves the current OS identifier at call time so reconnected devices
   * are automatically detected and reopened.
   */
  juce::MidiOutput* getOrOpenDevice(const std::string& deviceNameOrId);

  struct OpenDevice {
    std::string identifier;  // identifier used when opening (changes on reconnect)
    std::unique_ptr<juce::MidiOutput> output;
  };
  // Keyed by device name (stable across reconnects, unlike identifier)
  std::unordered_map<std::string, OpenDevice> openDevices;
};
