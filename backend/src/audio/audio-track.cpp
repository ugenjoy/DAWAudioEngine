#include "audio/audio-track.hpp"
#include <juce_audio_utils/juce_audio_utils.h>

AudioTrack::AudioTrack()
    : id(juce::Uuid().toDashedString().toStdString()),
      volume(0.4f),
      pan(0.0f),
      mute(false) {}

void AudioTrack::setMute(bool shouldMute) {
  this->mute = shouldMute;
}

void AudioTrack::setVolume(float newVolume) {
  this->volume = juce::jlimit(0.0f, 1.0f, newVolume);
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