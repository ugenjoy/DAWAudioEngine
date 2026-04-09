#include "services/midi-input-manager.hpp"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include "app-context.hpp"
#include "events/event-engine.hpp"

void MidiInputManager::init(EventEngine& engine, AppContext& ctx) {
  engine_ = &engine;
  ctx_    = &ctx;
  running_.store(true);

  auto devices = juce::MidiInput::getAvailableDevices();
  for (const auto& info : devices) {
    auto input = juce::MidiInput::openDevice(info.identifier, this);
    if (input) {
      input->start();
      juce::Logger::writeToLog("[MidiInputManager] Opened input: " + info.name);
      inputs_.push_back(std::move(input));
    } else {
      juce::Logger::writeToLog("[MidiInputManager] Failed to open: " + info.name);
    }
  }

  juce::Logger::writeToLog("[MidiInputManager] Listening on " +
                           juce::String((int)inputs_.size()) + " input(s)");
}

void MidiInputManager::shutdown() {
  running_.store(false);
  inputs_.clear();  // Calls MidiInput destructor which stops the I/O thread
  engine_ = nullptr;
  ctx_    = nullptr;
}

nlohmann::json MidiInputManager::getAvailableInputs() const {
  nlohmann::json result = nlohmann::json::array();
  for (const auto& info : juce::MidiInput::getAvailableDevices()) {
    nlohmann::json entry;
    entry["name"]       = info.name.toStdString();
    entry["identifier"] = info.identifier.toStdString();
    result.push_back(entry);
  }
  return result;
}

void MidiInputManager::handleIncomingMidiMessage(juce::MidiInput* source,
                                                  const juce::MidiMessage& msg) {
  if (!running_.load()) return;

  // Capture by value — message and device name are copied to the lambda
  std::string deviceName = source->getName().toStdString();

  if (msg.isNoteOn()) {
    juce::Logger::writeToLog(
        "[MidiInputManager] Note On — device: '" + juce::String(deviceName) +
        "' note: " + juce::String(msg.getNoteNumber()) +
        " ch: " + juce::String(msg.getChannel()) +
        " vel: " + juce::String(msg.getVelocity()));
  }

  juce::MessageManager::callAsync([this, msg, deviceName]() {
    if (!running_.load() || engine_ == nullptr || ctx_ == nullptr) return;
    engine_->fireMidi(msg, deviceName, *ctx_);
  });
}
