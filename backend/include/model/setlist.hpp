// backend/include/model/setlist.hpp
#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "events/event-rule.hpp"

enum class SetlistTransition { Stop, Pause, Continue };

struct SetlistEntry {
  std::string songId;
  SetlistTransition transition = SetlistTransition::Stop;

  nlohmann::json toJson() const;
  static SetlistEntry fromJson(const nlohmann::json& j);
};

class Setlist {
 public:
  const std::string& getId() const { return id; }
  const std::string& getName() const { return name; }
  void setName(const std::string& n) { name = n; }

  const std::vector<SetlistEntry>& getEntries() const { return entries; }
  void setEntries(std::vector<SetlistEntry> e) { entries = std::move(e); }

  const std::vector<EventRule>& getEventRules() const { return eventRules; }
  void setEventRules(std::vector<EventRule> rules) {
    eventRules = std::move(rules);
  }

  nlohmann::json toJson() const;
  static Setlist fromJson(const nlohmann::json& j);

  /** Generate a new setlist with a fresh UUID id. */
  static Setlist create(const std::string& name);

 private:
  std::string id;
  std::string name;
  std::vector<SetlistEntry> entries;
  std::vector<EventRule> eventRules;
};
