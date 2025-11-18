#include "audio-track.hpp"
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