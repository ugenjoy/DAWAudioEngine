#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
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

 private:
  std::vector<std::unique_ptr<AudioTrack>> tracks;
};