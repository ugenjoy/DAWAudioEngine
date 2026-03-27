#pragma once

#include <juce_core/juce_core.h>

#include <atomic>
#include <nlohmann/json.hpp>
#include <vector>

#include "audio/metronome-track.hpp"
#include "events/event-rule.hpp"
#include "tracks-manager.hpp"

enum class SongLoadState { MetadataOnly, Loading, Loaded };

class Song {
 public:
  Song();
  ~Song();

  void addTrack(std::unique_ptr<AudioTrack>);
  bool removeTrack(const std::string& trackId);

  void render(juce::AudioBuffer<float>& mixBuffer,
              juce::AudioBuffer<float>& trackBuffer,
              const juce::AudioBuffer<float>& inputBuffer, int numSamples,
              double position, bool isPlaying);

  void loadAudio(const std::string& audioDir);
  void unloadAudio();
  SongLoadState getLoadState() const { return loadState.load(); }

  // Serialization
  nlohmann::json toJson() const;
  static std::unique_ptr<Song> fromJson(const nlohmann::json& j,
                                        const std::string& audioDir,
                                        bool loadAudio = true);

  // Setters / Getters
  std::string getId() const { return id; }
  std::string getName() const { return name; }
  void setName(const std::string& n) { name = n; }

  void sampleRateChanged();

  void freezeAllTracks(float tempo, double sampleRate);
  void unfreezeAllTracks();

  float getTempo() const { return tempo; }
  void setTempo(float newTempo) { tempo = newTempo; }

  TracksManager* getTracksManager() const { return tracksManager.get(); }
  MetronomeTrack* getMetronomeTrack() const { return metronomeTrack.get(); }

  // Event rules (song-level)
  const std::vector<EventRule>& getEventRules() const { return eventRules; }
  void setEventRules(std::vector<EventRule> rules) {
    eventRules = std::move(rules);
  }
  void addEventRule(EventRule rule) { eventRules.push_back(std::move(rule)); }
  bool removeEventRule(const std::string& ruleId);
  bool updateEventRule(const std::string& ruleId, const EventRule& updated);

 private:
  std::string id;
  std::string name;
  float tempo;

  std::unique_ptr<TracksManager> tracksManager;
  std::unique_ptr<MetronomeTrack> metronomeTrack;
  std::vector<EventRule> eventRules;

  std::atomic<SongLoadState> loadState{SongLoadState::MetadataOnly};
};