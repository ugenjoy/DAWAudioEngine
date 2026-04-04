// backend/src/model/setlist.cpp
#include "model/setlist.hpp"
#include <juce_core/juce_core.h>

// ── SetlistEntry ──────────────────────────────────────────────────────────

nlohmann::json SetlistEntry::toJson() const {
  return {
    {"songId", songId},
    {"transition", transition == SetlistTransition::Continue ? "continue" : "stop"}
  };
}

SetlistEntry SetlistEntry::fromJson(const nlohmann::json& j) {
  SetlistEntry e;
  e.songId = j.value("songId", "");
  std::string t = j.value("transition", "stop");
  e.transition = (t == "continue") ? SetlistTransition::Continue
                                   : SetlistTransition::Stop;
  return e;
}

// ── Setlist ───────────────────────────────────────────────────────────────

Setlist Setlist::create(const std::string& name) {
  Setlist s;
  s.id = juce::Uuid().toString().toStdString();
  s.name = name;
  return s;
}

nlohmann::json Setlist::toJson() const {
  nlohmann::json entriesJson = nlohmann::json::array();
  for (const auto& e : entries) entriesJson.push_back(e.toJson());

  nlohmann::json rulesJson = nlohmann::json::array();
  for (const auto& r : eventRules) rulesJson.push_back(r.toJson());

  return {{"id", id}, {"name", name},
          {"entries", entriesJson}, {"events", rulesJson}};
}

Setlist Setlist::fromJson(const nlohmann::json& j) {
  Setlist s;
  s.id = j.value("id", "");
  s.name = j.value("name", "");

  if (j.contains("entries") && j["entries"].is_array()) {
    for (const auto& e : j["entries"])
      s.entries.push_back(SetlistEntry::fromJson(e));
  }

  if (j.contains("events") && j["events"].is_array()) {
    for (const auto& r : j["events"])
      s.eventRules.push_back(EventRule::fromJson(r));
  }

  return s;
}
