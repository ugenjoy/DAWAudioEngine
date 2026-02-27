#include "commands/midi-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "commands/command-factory.hpp"
#include "services/midi-output-manager.hpp"
#include "websocket/websocket-server.hpp"

// ── MidiListOutputsCommand ─────────────────────────────────────────────────

void MidiListOutputsCommand::execute(AppContext& ctx) {
  auto& midiManager = ctx.getMidiOutputManager();
  nlohmann::json outputs = midiManager.getAvailableOutputs();

  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "midi.outputsListed";
  broadcast["outputs"] = outputs;

  ctx.getWebSocketServer().broadcast(broadcast.dump());
  juce::Logger::writeToLog("[MidiListOutputsCommand] Listed " +
                           juce::String((int)outputs.size()) +
                           " MIDI output(s)");
}

// ── MidiSendCommand ────────────────────────────────────────────────────────

MidiSendCommand::MidiSendCommand(std::string deviceIdentifier,
                                 std::vector<uint8_t> bytes)
    : deviceIdentifier(std::move(deviceIdentifier)),
      bytes(std::move(bytes)) {}

void MidiSendCommand::execute(AppContext& ctx) {
  if (bytes.empty()) {
    juce::Logger::writeToLog(
        "[MidiSendCommand] Empty message, ignoring");
    return;
  }

  // Build a raw MIDI message from bytes
  juce::MidiMessage message;
  if (bytes.size() == 1) {
    message = juce::MidiMessage(bytes[0]);
  } else if (bytes.size() == 2) {
    message = juce::MidiMessage(bytes[0], bytes[1]);
  } else {
    message = juce::MidiMessage(bytes[0], bytes[1], bytes[2]);
  }

  bool ok = ctx.getMidiOutputManager().sendMessage(deviceIdentifier, message);

  juce::Logger::writeToLog(
      "[MidiSendCommand] Send to '" + juce::String(deviceIdentifier) +
      "': " + (ok ? "OK" : "FAILED"));
}

// ── Auto-registration ──────────────────────────────────────────────────────

REGISTER_COMMAND("midi.listOutputs", MidiListOutputsCommand);

REGISTER_COMMAND_WITH_CREATOR(
    "midi.send", MidiSend,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string device = payload.value("device", "");
      if (device.empty() || !payload.contains("message") ||
          !payload["message"].is_array()) {
        return nullptr;
      }

      std::vector<uint8_t> bytes;
      for (const auto& b : payload["message"]) {
        bytes.push_back(static_cast<uint8_t>(b.get<int>()));
      }
      if (bytes.empty()) return nullptr;

      return std::make_unique<MidiSendCommand>(std::move(device),
                                               std::move(bytes));
    });
