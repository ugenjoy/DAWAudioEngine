#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "commands/command.hpp"

/**
 * Command to list available MIDI output devices.
 * Available in both Live and Edit modes.
 */
class MidiListOutputsCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Command to list available MIDI input devices.
 * Payload: none
 * Broadcasts: midi.inputsListed { inputs: [{name, identifier}] }
 */
class MidiListInputsCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Command to send a raw MIDI message to a device.
 * Payload: { "device": "<identifier>", "message": [statusByte, data1, data2?] }
 * Available in both Live and Edit modes (for testing event configs).
 */
class MidiSendCommand : public Command {
 public:
  MidiSendCommand(std::string deviceIdentifier, std::vector<uint8_t> bytes);
  void execute(AppContext& ctx) override;

 private:
  std::string deviceIdentifier;
  std::vector<uint8_t> bytes;
};
