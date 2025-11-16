#include "tracks-manager.hpp"

TracksManager::TracksManager() {}
TracksManager::~TracksManager() {}

void TracksManager::addTrack(std::unique_ptr<AudioTrack> track) {
  tracks.push_back(std::move(track));
}

void TracksManager::removeTrack() {
  // To implement
}

void TracksManager::renderTracks(juce::AudioBuffer<float>& mixBuffer,
                                 juce::AudioBuffer<float>& trackBuffer,
                                 int numSamples,
                                 double currentPosition) {
  for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
    trackBuffer.clear();

    tracks[trackIdx]->renderBlock(trackBuffer, 0, numSamples, currentPosition);

    for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel) {
      mixBuffer.addFrom(channel, 0, trackBuffer, 0, 0, numSamples);
    }
  }
}