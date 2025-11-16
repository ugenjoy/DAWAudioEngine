#include "tracks-manager.hpp"

TracksManager::TracksManager() = default;
TracksManager::~TracksManager() = default;

void TracksManager::addTrack(std::unique_ptr<AudioTrack> track) {
  tracks.push_back(std::move(track));
}

void TracksManager::removeTrack() {
  // To implement
}

void TracksManager::renderTracks(juce::AudioBuffer<float>& mixBuffer,
                                 juce::AudioBuffer<float>& trackBuffer,
                                 int numSamples,
                                 double currentPosition,
                                 float tempo) {
  for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
    trackBuffer.clear();

    if (!tracks[trackIdx]->mute) {
      tracks[trackIdx]->renderBlock(trackBuffer, 0, numSamples, currentPosition,
                                    tempo);
    }

    for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel) {
      mixBuffer.addFrom(channel, 0, trackBuffer, 0, 0, numSamples);
    }
  }
}