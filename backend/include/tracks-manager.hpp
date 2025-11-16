#pragma once
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
                    double currentPosition);

 private:
  std::vector<std::unique_ptr<AudioTrack>> tracks;
};