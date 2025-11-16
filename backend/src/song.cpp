#include "song.hpp"
#include "audio-context.hpp"

Song::Song()
    : tempo(120.0f),
      currentPosition(0.0),
      tracksManager(std::make_unique<TracksManager>()) {}

Song::~Song() = default;

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
                              currentPosition, tempo);

  auto const& ctx = AudioContext::getInstance();
  currentPosition += (double)numSamples / ctx.sampleRate;
}