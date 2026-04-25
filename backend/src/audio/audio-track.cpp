#include "audio/audio-track.hpp"

#include <juce_audio_utils/juce_audio_utils.h>

namespace {
int nextColor() {
  static int counter = 0;
  return (counter++ % 8) + 1;
}
}  // namespace

AudioTrack::AudioTrack()
    : id(juce::Uuid().toDashedString().toStdString()),
      volume(0.0f),
      pan(0.0f),
      mute(false),
      solo(false),
      color(nextColor()) {}

void AudioTrack::setMute(bool shouldMute) {
  this->mute = shouldMute;
}

void AudioTrack::setSolo(bool shouldSolo) {
  this->solo = shouldSolo;
}

void AudioTrack::setVolume(float newVolume) {
  this->volume = juce::jlimit(-80.0f, 12.0f, newVolume);
}

void AudioTrack::setInputChannel(int channel) {
  inputChannel.store(channel, std::memory_order_relaxed);
}

int AudioTrack::getInputChannel() const {
  return inputChannel.load(std::memory_order_relaxed);
}

void AudioTrack::setInputStereo(bool stereo) {
  inputStereo.store(stereo, std::memory_order_relaxed);
}

bool AudioTrack::isInputStereo() const {
  return inputStereo.load(std::memory_order_relaxed);
}

void AudioTrack::setMonitoring(bool enabled) {
  monitoring.store(enabled, std::memory_order_relaxed);
}

bool AudioTrack::isMonitoring() const {
  return monitoring.load(std::memory_order_relaxed);
}

void AudioTrack::unfreeze() {
  frozen.store(false, std::memory_order_release);
  frozenPeriodSamples = 0;
  frozenBuffer.setSize(0, 0);
}