#include "events/event-rule.hpp"

#include <juce_core/juce_core.h>

// ── EventAction ────────────────────────────────────────────────────────────

nlohmann::json EventAction::toJson() const {
  nlohmann::json j;
  j["type"] = type;
  j["params"] = params;
  return j;
}

EventAction EventAction::fromJson(const nlohmann::json& j) {
  EventAction a;
  a.type = j.value("type", "");
  a.params = j.contains("params") ? j["params"] : nlohmann::json::object();
  return a;
}

// ── EventRule ──────────────────────────────────────────────────────────────

nlohmann::json EventRule::toJson() const {
  nlohmann::json j;
  j["id"] = id;
  j["trigger"] = trigger;
  j["action"] = action.toJson();
  j["enabled"] = enabled;
  return j;
}

EventRule EventRule::fromJson(const nlohmann::json& j) {
  EventRule r;
  // Generate a new UUID if none provided
  r.id = j.value("id", juce::Uuid().toDashedString().toStdString());
  r.trigger = j.value("trigger", "");
  if (j.contains("action")) {
    r.action = EventAction::fromJson(j["action"]);
  }
  r.enabled = j.value("enabled", true);
  return r;
}
