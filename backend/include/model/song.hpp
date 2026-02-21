#pragma once

#include <juce_core/juce_core.h>

#include <atomic>
#include <nlohmann/json.hpp>

#include "tracks-manager.hpp"

class Song {
 public:
  Song();
  ~Song();

  void addTrack(std::unique_ptr<AudioTrack>);
  void removeTrack();

  void render(juce::AudioBuffer<float>& mixBuffer,
              juce::AudioBuffer<float>& trackBuffer, int numSamples,
              double position);

  // Serialization
  nlohmann::json toJson() const;
  static std::unique_ptr<Song> fromJson(const nlohmann::json& j);

  // Setters / Getters
  std::string getId() const { return id; }

  float getTempo() const { return tempo; }
  void setTempo(float newTempo) { tempo = newTempo; }

  TracksManager* getTracksManager() const { return tracksManager.get(); }

 private:
  std::string id;
  std::string name;
  float tempo;

  std::unique_ptr<TracksManager> tracksManager;
};