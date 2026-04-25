#include "commands/midi-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "commands/command-factory.hpp"
#include "services/midi-input-manager.hpp"
#include "services/midi-output-manager.hpp"
#include "services/midi-utils.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── MidiListOutputsCommand ─────────────────────────────────────────────────

void MidiListOutputsCommand::execute(AppContext& ctx) {
  auto& midiManager = ctx.getMidiOutputManager();
  nlohmann::json outputs = midiManager.getAvailableOutputs();

  broadcast::send(ctx.getWebSocketServer(), "midi.outputsListed",
                  {{"outputs", outputs}});
  juce::Logger::writeToLog("[MidiListOutputsCommand] Listed " +
                           juce::String((int)outputs.size()) +
                           " MIDI output(s)");
}

REGISTER_COMMAND("midi.listOutputs", MidiListOutputsCommand);

// ── MidiListInputsCommand ──────────────────────────────────────────────────

void MidiListInputsCommand::execute(AppContext& ctx) {
  nlohmann::json inputs = ctx.getMidiInputManager().getAvailableInputs();
  broadcast::send(ctx.getWebSocketServer(), "midi.inputsListed",
                  {{"inputs", inputs}});
  juce::Logger::writeToLog("[MidiListInputsCommand] Listed " +
                           juce::String((int)inputs.size()) +
                           " MIDI input(s)");
}

REGISTER_COMMAND("midi.listInputs", MidiListInputsCommand);

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

  auto message = midi::fromBytes(bytes);

  bool ok = ctx.getMidiOutputManager().sendMessage(deviceIdentifier, message);

  juce::Logger::writeToLog(
      "[MidiSendCommand] Send to '" + juce::String(deviceIdentifier) +
      "': " + (ok ? "OK" : "FAILED"));
}

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
