#include <juce_core/juce_core.h>

#include "app-context.hpp"
#include "events/action-executor.hpp"
#include "services/midi-output-manager.hpp"

/**
 * Executes "midi.send" actions.
 * Expected params: { "device": "<identifier>", "message": [statusByte, data1, data2?] }
 */
class MidiActionExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "midi.send"; }

  void execute(const EventAction& action, AppContext& ctx) override {
    const auto& params = action.params;

    if (!params.contains("device") || !params.contains("message") ||
        !params["message"].is_array()) {
      juce::Logger::writeToLog(
          "[MidiActionExecutor] Invalid params: missing device or message");
      return;
    }

    std::string device = params["device"].get<std::string>();
    std::vector<uint8_t> bytes;
    for (const auto& b : params["message"]) {
      bytes.push_back(static_cast<uint8_t>(b.get<int>()));
    }

    if (bytes.empty()) {
      juce::Logger::writeToLog(
          "[MidiActionExecutor] Empty message bytes, skipping");
      return;
    }

    juce::MidiMessage message;
    if (bytes.size() == 1) {
      message = juce::MidiMessage(bytes[0]);
    } else if (bytes.size() == 2) {
      message = juce::MidiMessage(bytes[0], bytes[1]);
    } else {
      message = juce::MidiMessage(bytes[0], bytes[1], bytes[2]);
    }

    bool ok = ctx.getMidiOutputManager().sendMessage(device, message);
    juce::Logger::writeToLog(
        "[MidiActionExecutor] Send to '" + juce::String(device) +
        "': " + (ok ? "OK" : "FAILED"));
  }
};

// Factory function used by EventEngine setup in main.cpp
std::unique_ptr<ActionExecutor> createMidiActionExecutor() {
  return std::make_unique<MidiActionExecutor>();
}
