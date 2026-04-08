#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "events/action-executor.hpp"
#include "events/event-rule.hpp"
#include "model/marker.hpp"

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
   * Update the marker position map used for resolving position triggers and seek actions.
   * Must be called after song load and after any marker CRUD operation.
   */
  void loadMarkers(const std::vector<Marker>& markers);

  /**
   * Resolve a marker id to its position in seconds.
   * Returns nullopt if the marker id is not found.
   */
  std::optional<double> resolveMarker(const std::string& markerId) const;

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

  /**
   * Returns true if the given position window ]prevPos, currentPos] contains
   * the rule's trigger position (resolved via the internal marker map).
   * Pure matching logic — no side effects. Ignores the enabled flag.
   */
  bool matchesPositionTrigger(const EventRule& rule,
                               double prevPos,
                               double currentPos) const;

  /**
   * Called by the audio engine on each timer callback (JUCE message thread).
   * Fires all enabled position-triggered rules whose position falls in
   * ]prevPos, currentPos].
   * @param prevPos Position (in seconds) at the start of the timer interval
   * @param currentPos Position (in seconds) at the end of the timer interval
   */
  void firePosition(double prevPos, double currentPos, AppContext& ctx);

 private:
  std::unordered_map<std::string, std::unique_ptr<ActionExecutor>> executors;
  std::vector<EventRule> rules;  // Combined project + setlist + song rules
  std::unordered_map<std::string, double> markerPositions;
};
