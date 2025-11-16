#pragma once

#include "tracks-manager.hpp"

class Song {
 public:
  Song();
  ~Song();

  void addTrack(std::unique_ptr<AudioTrack>);
  void removeTrack();

  void render(juce::AudioBuffer<float>& mixBuffer,
              juce::AudioBuffer<float>& trackBuffer,
              int numSamples);

  // Setters / Getters
  float getTempo() const { return tempo; }
  void setTempo(float newTempo) { tempo = newTempo; }

  double getCurrentPosition() const { return currentPosition; }
  void setCurrentPosition(double pos) { currentPosition = pos; }

 private:
  float tempo;
  double currentPosition;

  std::unique_ptr<TracksManager> tracksManager;
};