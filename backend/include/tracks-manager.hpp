#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <nlohmann/json.hpp>
#include "audio-track.hpp"

class TracksManager {
 public:
  TracksManager();
  ~TracksManager();

  void addTrack(std::unique_ptr<AudioTrack>);
  void removeTrack();
  void getTrackList();

  void renderTracks(juce::AudioBuffer<float>& mixBuffer,
                    juce::AudioBuffer<float>& trackBuffer,
                    int numSamples,
                    double currentPosition,
                    float tempo);

  // Serialization
  nlohmann::json toJson() const;
  void loadFromJson(const nlohmann::json& j);

  // Getter for tracks
  const std::vector<std::unique_ptr<AudioTrack>>& getTracks() const { return tracks; }

 private:
  std::vector<std::unique_ptr<AudioTrack>> tracks;
};