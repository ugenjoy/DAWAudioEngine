#include "song.hpp"
#include "audio-context.hpp"

Song::Song()
    : currentPosition(0.0), tracksManager(std::make_unique<TracksManager>()) {}

Song::~Song() {}

void Song::addTrack(std::unique_ptr<AudioTrack> track) {
  tracksManager->addTrack(std::move(track));
}

void Song::removeTrack() {
  // To implement
}

void Song::render(juce::AudioBuffer<float>& mixBuffer,
                  juce::AudioBuffer<float>& trackBuffer,
                  int numSamples) {
  tracksManager->renderTracks(mixBuffer, trackBuffer, numSamples,
                              currentPosition);

  auto const& ctx = AudioContext::getInstance();
  currentPosition += (double)numSamples / ctx.sampleRate;
}