#pragma once

#include "tracks-manager.hpp"
#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>

class Song {
 public:
  Song();
  ~Song();

  void addTrack(std::unique_ptr<AudioTrack>);
  void removeTrack();

  void render(juce::AudioBuffer<float>& mixBuffer,
              juce::AudioBuffer<float>& trackBuffer,
              int numSamples);

  // Serialization
  nlohmann::json toJson() const;
  static std::unique_ptr<Song> fromJson(const nlohmann::json& j);

  // Setters / Getters
  std::string getId() const { return id; }

  float getTempo() const { return tempo; }
  void setTempo(float newTempo) { tempo = newTempo; }

  double getCurrentPosition() const { return currentPosition; }
  void setCurrentPosition(double pos) { currentPosition = pos; }

  TracksManager* getTracksManager() const { return tracksManager.get(); }

 private:
  std::string id;
  float tempo;
  double currentPosition;

  std::unique_ptr<TracksManager> tracksManager;
};