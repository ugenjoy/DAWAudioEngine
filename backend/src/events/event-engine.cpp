#include "events/event-engine.hpp"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "model/marker.hpp"

void EventEngine::registerExecutor(std::unique_ptr<ActionExecutor> executor) {
  std::string type = executor->getActionType();
  executors[type] = std::move(executor);
}

void EventEngine::loadRules(const std::vector<EventRule>& projectRules,
                            const std::vector<EventRule>& setlistRules,
                            const std::vector<EventRule>& songRules) {
  {
    juce::SpinLock::ScopedLockType lock(dataLock);
    rules.clear();
    rules.insert(rules.end(), projectRules.begin(), projectRules.end());
    rules.insert(rules.end(), setlistRules.begin(), setlistRules.end());
    rules.insert(rules.end(), songRules.begin(), songRules.end());
  }
  juce::Logger::writeToLog(
      "[EventEngine] Loaded " + juce::String((int)rules.size()) +
      " rule(s) (" + juce::String((int)projectRules.size()) + " project, " +
      juce::String((int)setlistRules.size()) + " setlist, " +
      juce::String((int)songRules.size()) + " song)");
}

void EventEngine::clearRules() {
  juce::SpinLock::ScopedLockType lock(dataLock);
  rules.clear();
}

void EventEngine::loadMarkers(const std::vector<Marker>& markers) {
  {
    juce::SpinLock::ScopedLockType lock(dataLock);
    markerPositions.clear();
    for (const auto& m : markers) {
      markerPositions[m.id] = m.position;
    }
  }
  juce::Logger::writeToLog(
      "[EventEngine] Loaded " + juce::String((int)markerPositions.size()) +
      " marker(s)");
}

std::optional<double> EventEngine::resolveMarker(const std::string& markerId) const {
  auto it = markerPositions.find(markerId);
  if (it == markerPositions.end()) return std::nullopt;
  return it->second;
}

void EventEngine::fire(const std::string& trigger, AppContext& ctx) {
  // Snapshot rules to guard against re-entrant loadRules() calls from executors
  // (e.g. setlist.next → changeSong → loadRules) which would otherwise clear
  // and resize the vector while we are still iterating over it.
  const std::vector<EventRule> snapshot = rules;

  int fired = 0;
  for (const auto& rule : snapshot) {
    if (!rule.enabled || rule.trigger != trigger) continue;

    // Skip gracefully if no executor is registered for this action type.
    // This allows rules to reference future action types without crashing.
    auto it = executors.find(rule.action.type);
    if (it == executors.end()) {
      juce::Logger::writeToLog(
          "[EventEngine] No executor for action type: " +
          juce::String(rule.action.type));
      continue;
    }

    juce::Logger::writeToLog(
        "[EventEngine] Firing rule '" + juce::String(rule.id) +
        "' → " + juce::String(rule.action.type));
    it->second->execute(rule.action, ctx);
    ++fired;
  }

  if (fired > 0) {
    juce::Logger::writeToLog(
        "[EventEngine] Fired " + juce::String(fired) +
        " rule(s) for trigger '" + juce::String(trigger) + "'");
  }
}

bool EventEngine::matchesMidiTrigger(const EventRule& rule,
                                      const juce::MidiMessage& msg,
                                      const std::string& deviceName) {
  const auto& p = rule.triggerParams;

  if (rule.trigger == "midi.note") {
    if (!msg.isNoteOn()) return false;
    int ruleNote    = p.value("note",    -1);
    int ruleChannel = p.value("channel", 0);
    std::string dev = p.value("device",  std::string(""));
    return (ruleNote    < 0 || ruleNote    == msg.getNoteNumber()) &&
           (ruleChannel == 0 || ruleChannel == msg.getChannel())   &&
           (dev.empty()      || dev         == deviceName);
  }

  if (rule.trigger == "midi.cc") {
    if (!msg.isController()) return false;
    int ruleCc      = p.value("cc",       -1);
    int ruleChannel = p.value("channel",   0);
    int threshold   = p.value("threshold", 0);
    std::string dev = p.value("device",    std::string(""));
    return (ruleCc      < 0 || ruleCc      == msg.getControllerNumber()) &&
           (ruleChannel == 0 || ruleChannel == msg.getChannel())          &&
           (dev.empty()      || dev         == deviceName)                &&
           (msg.getControllerValue() >= threshold);
  }

  return false;
}

bool EventEngine::matchesPositionTrigger(const EventRule& rule,
                                          double prevPos,
                                          double currentPos) const {
  if (rule.trigger != "position") return false;
  std::string markerId = rule.triggerParams.value("markerId", std::string(""));
  if (markerId.empty()) return false;
  auto pos = resolveMarker(markerId);
  if (!pos.has_value()) {
    juce::Logger::writeToLog(
        "[EventEngine] matchesPositionTrigger: marker '" +
        juce::String(markerId) + "' not found, skipping");
    return false;
  }
  return prevPos < pos.value() && pos.value() <= currentPos;
}

std::optional<double> EventEngine::firePosition(double prevPos, double currentPos, AppContext& ctx) {
  // Snapshot rules for the same reason as fire() — executors can call loadRules().
  const std::vector<EventRule> snapshot = rules;

  std::optional<double> pauseSnapPos;

  for (const auto& rule : snapshot) {
    if (!rule.enabled) continue;
    if (!matchesPositionTrigger(rule, prevPos, currentPos)) continue;
    auto it = executors.find(rule.action.type);
    if (it == executors.end()) {
      continue;
    }
    it->second->execute(rule.action, ctx);

    // After pause: record the exact trigger position so the caller
    // (timerCallback) can snap the playhead to it, compensating for the
    // ~100ms drift introduced by the 10 Hz timer resolution.
    if (rule.action.type == "transport.pause") {
      std::string markerId =
          rule.triggerParams.value("markerId", std::string(""));
      auto pos = resolveMarker(markerId);
      if (pos.has_value()) {
        pauseSnapPos = pos;
      }
    }
  }

  return pauseSnapPos;
}

std::optional<double> EventEngine::checkSeekTrigger(double prevPos,
                                                    double currPos) const {
  const juce::SpinLock::ScopedTryLockType lock(dataLock);
  if (!lock.isLocked()) return std::nullopt;

  for (const auto& rule : rules) {
    if (!rule.enabled) continue;
    if (rule.trigger != "position") continue;
    if (rule.action.type != "transport.seekToPosition") continue;

    std::string triggerMarkerId =
        rule.triggerParams.value("markerId", std::string(""));
    if (triggerMarkerId.empty()) continue;

    auto it = markerPositions.find(triggerMarkerId);
    if (it == markerPositions.end()) continue;
    double triggerPos = it->second;
    if (!(prevPos < triggerPos && triggerPos <= currPos)) continue;

    // Trigger matched — resolve the seek target marker
    std::string targetMarkerId =
        rule.action.params.value("markerId", std::string(""));
    if (targetMarkerId.empty()) continue;

    auto tit = markerPositions.find(targetMarkerId);
    if (tit == markerPositions.end()) continue;
    return tit->second;
  }
  return std::nullopt;
}

void EventEngine::fireMidi(const juce::MidiMessage& msg,
                            const std::string& deviceName,
                            AppContext& ctx) {
  // Snapshot rules before iterating: an executor such as setlist.next can call
  // loadRules() via changeSong(), which clears and resizes the member vector
  // while we are iterating over it.  Without a snapshot the iterator held by
  // the range-for loop becomes invalid (the vector shrinks from N to M < N,
  // positions M..N-1 are destroyed, but __end still points to the old end),
  // leading to UB and a json type_error when destroyed EventRule objects are
  // read past the new end of the vector.
  const std::vector<EventRule> snapshot = rules;

  for (const auto& rule : snapshot) {
    if (!rule.enabled) continue;
    if (!matchesMidiTrigger(rule, msg, deviceName)) continue;

    auto it = executors.find(rule.action.type);
    if (it == executors.end()) {
      juce::Logger::writeToLog("[EventEngine] No executor for MIDI action: " +
                               juce::String(rule.action.type));
      continue;
    }
    juce::Logger::writeToLog("[EventEngine] MIDI trigger matched rule '" +
                             juce::String(rule.id) + "' → " +
                             juce::String(rule.action.type));
    it->second->execute(rule.action, ctx);
  }
}
