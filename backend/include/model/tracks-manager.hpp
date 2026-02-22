#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <nlohmann/json.hpp>

#include "audio/audio-track.hpp"

class TracksManager {
 public:
  TracksManager();
  ~TracksManager();

  void addTrack(std::unique_ptr<AudioTrack>);
  void removeTrack();
  void getTrackList();

  void renderTracks(juce::AudioBuffer<float>& mixBuffer,
                    juce::AudioBuffer<float>& trackBuffer,
                    const juce::AudioBuffer<float>& inputBuffer,
                    int numSamples, double currentPosition, float tempo,
                    bool isPlaying);

  AudioTrack* findTrackById(const std::string& id) const;
  void sampleRateChanged();

  /** Pre-render all freezable tracks (e.g., BeatTrack) into frozen buffers. */
  void freezeAll(float tempo, double sampleRate);

  /** Release all frozen buffers and resume live rendering. */
  void unfreezeAll();

  // Serialization
  nlohmann::json toJson() const;
  void loadFromJson(const nlohmann::json& j);

  // Getter for tracks
  const std::vector<std::unique_ptr<AudioTrack>>& getTracks() const {
    return tracks;
  }

 private:
  std::vector<std::unique_ptr<AudioTrack>> tracks;
};