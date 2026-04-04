#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <nlohmann/json.hpp>
#include <optional>

#include "audio/audio-track.hpp"
#include "events/event-rule.hpp"
#include "model/song.hpp"

class SongsManager {
 public:
  SongsManager();
  ~SongsManager();

  void addSong(std::unique_ptr<Song>);
  bool removeSong(const std::string& uuid);
  std::vector<Song*> getSongList();
  Song* getSong(int songId);
  Song* getSongById(const std::string& uuid);
  bool renameSong(const std::string& uuid, const std::string& name);
  bool reorderSong(const std::string& uuid, int newIndex);
  bool setEndPosition(const std::string& uuid, std::optional<double> pos);

  // Serialization
  nlohmann::json toJson() const;
  void loadFromJson(const nlohmann::json& j);

  // Project-level event rules
  const std::vector<EventRule>& getProjectEventRules() const {
    return projectEventRules;
  }
  void setProjectEventRules(std::vector<EventRule> rules) {
    projectEventRules = std::move(rules);
  }
  void addProjectEventRule(EventRule rule) {
    projectEventRules.push_back(std::move(rule));
  }
  bool removeProjectEventRule(const std::string& ruleId);
  bool updateProjectEventRule(const std::string& ruleId,
                              const EventRule& updated);

  // Project-level events serialization (separate from songs array)
  nlohmann::json projectEventsToJson() const;
  void loadProjectEventsFromJson(const nlohmann::json& j);

 private:
  std::vector<std::unique_ptr<Song>> songs;
  std::vector<EventRule> projectEventRules;
};