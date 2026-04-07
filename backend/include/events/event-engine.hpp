#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "events/action-executor.hpp"
#include "events/event-rule.hpp"

class AppContext;

/**
 * Central event dispatch engine.
 * Stores event rules (project-level and song-level) and dispatches
 * trigger events to the appropriate ActionExecutors.
 */
class EventEngine {
 public:
  EventEngine() = default;
  ~EventEngine() = default;

  /**
   * Register an executor for a specific action type.
   * Must be called before firing events.
   */
  void registerExecutor(std::unique_ptr<ActionExecutor> executor);

  /**
   * Load rules for the current context.
   * Replaces any previously loaded rules.
   * @param projectRules Rules defined at the project level
   * @param setlistRules Rules defined at the active setlist level
   * @param songRules Rules defined at the current song level
   */
  void loadRules(const std::vector<EventRule>& projectRules,
                 const std::vector<EventRule>& setlistRules,
                 const std::vector<EventRule>& songRules);

  /**
   * Clear all loaded rules (e.g. when unloading a project).
   */
  void clearRules();

  /**
   * Fire a trigger, executing all matching enabled rules.
   * @param trigger The trigger name (e.g. "song.loaded")
   * @param ctx Application context passed to executors
   */
  void fire(const std::string& trigger, AppContext& ctx);

  /**
   * Returns true if the given MIDI message satisfies the rule's trigger.
   * Supports "midi.note" (Note On) and "midi.cc" (CC with threshold) triggers.
   * Pure matching logic — no side effects.
   */
  static bool matchesMidiTrigger(const EventRule& rule,
                                  const juce::MidiMessage& msg,
                                  const std::string& deviceName);

  /**
   * Called by MidiInputManager on the message thread.
   * Finds all rules whose MIDI trigger matches the message and executes them.
   */
  void fireMidi(const juce::MidiMessage& msg,
                const std::string& deviceName,
                AppContext& ctx);

 private:
  std::unordered_map<std::string, std::unique_ptr<ActionExecutor>> executors;
  std::vector<EventRule> rules;  // Combined project + setlist + song rules
};
