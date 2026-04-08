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
  rules.clear();
  rules.insert(rules.end(), projectRules.begin(), projectRules.end());
  rules.insert(rules.end(), setlistRules.begin(), setlistRules.end());
  rules.insert(rules.end(), songRules.begin(), songRules.end());

  juce::Logger::writeToLog(
      "[EventEngine] Loaded " + juce::String((int)rules.size()) +
      " rule(s) (" + juce::String((int)projectRules.size()) + " project, " +
      juce::String((int)setlistRules.size()) + " setlist, " +
      juce::String((int)songRules.size()) + " song)");
}

void EventEngine::clearRules() {
  rules.clear();
}

void EventEngine::loadMarkers(const std::vector<Marker>& markers) {
  markerPositions.clear();
  for (const auto& m : markers) {
    markerPositions[m.id] = m.position;
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
  int fired = 0;
  for (const auto& rule : rules) {
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

void EventEngine::firePosition(double prevPos, double currentPos, AppContext& ctx) {
  for (const auto& rule : rules) {
    if (!rule.enabled) continue;
    if (!matchesPositionTrigger(rule, prevPos, currentPos)) continue;
    auto it = executors.find(rule.action.type);
    if (it == executors.end()) {
      continue;
    }
    it->second->execute(rule.action, ctx);
  }
}

void EventEngine::fireMidi(const juce::MidiMessage& msg,
                            const std::string& deviceName,
                            AppContext& ctx) {
  for (const auto& rule : rules) {
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
