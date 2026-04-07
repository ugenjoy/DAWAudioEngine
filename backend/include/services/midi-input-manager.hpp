#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>
#include <nlohmann/json.hpp>

class AppContext;
class EventEngine;

/**
 * Listens to all available MIDI input devices and forwards incoming messages
 * to EventEngine::fireMidi() on the JUCE message thread.
 *
 * Call init() once AppContext is ready, shutdown() before destroying AppContext.
 */
class MidiInputManager : public juce::MidiInputCallback {
 public:
  MidiInputManager() = default;
  ~MidiInputManager() override { shutdown(); }

  /**
   * Open all available MIDI inputs and start listening.
   * Must be called after AppContext is fully constructed.
   */
  void init(EventEngine& engine, AppContext& ctx);

  /**
   * Stop all MIDI inputs. Safe to call multiple times.
   */
  void shutdown();

  /**
   * Returns a JSON array of available MIDI input devices.
   * Each entry has "name" and "identifier" fields.
   */
  nlohmann::json getAvailableInputs() const;

  // juce::MidiInputCallback — called on MIDI I/O thread
  void handleIncomingMidiMessage(juce::MidiInput* source,
                                  const juce::MidiMessage& message) override;

 private:
  EventEngine* engine_ = nullptr;
  AppContext* ctx_ = nullptr;
  std::atomic<bool> running_{false};
  std::vector<std::unique_ptr<juce::MidiInput>> inputs_;
};
