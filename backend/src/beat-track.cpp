#include "beat-track.hpp"
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio-context.hpp"

WaveTable BeatTrack::waveTable(WaveTable::SINE, 2048);

BeatTrack::BeatTrack(float frequency)
    : AudioTrack(), frequency(frequency), duration(0.15f) {
  // Enveloppe
  att = 0.01f;
  dec = 0.02f;
  sus = 0.8f;
  rel = 0.02f;
}

BeatTrack::~BeatTrack() {}

float BeatTrack::getSampleValue(double sampleTime) {
  if (mute) {
    return 0.0f;
  }

  auto& ctx = AudioContext::getInstance();
  float pi = juce::MathConstants<float>::pi;
  float currentTempo = ctx.tempoBPM.load();
  float interval = 60.0f / currentTempo;
  float timeSinceLastBeat = std::fmod(sampleTime, interval);

  if (timeSinceLastBeat < duration + rel) {
    float enveloppeVolume = computeEnveloppe(timeSinceLastBeat);
    float currentPhase = 2.0f * pi * frequency * timeSinceLastBeat;
    float sampleValue =
        enveloppeVolume * volume * waveTable.getSampleFast(currentPhase);

    return sampleValue;
  }

  return 0.0f;
}

void BeatTrack::setMute(bool mute) {
  this->mute = mute;
}

void BeatTrack::setVolume(float volume) {
  this->volume = juce::jlimit(0.0f, 1.0f, volume);
}

float BeatTrack::computeEnveloppe(float timeSinceLastBeat) {
  float enveloppeVolume = 0.0f;

  if (timeSinceLastBeat < duration) {
    if (timeSinceLastBeat < att) {  // Attack
      enveloppeVolume = timeSinceLastBeat / att;
      enveloppeVolume = juce::jmin(enveloppeVolume, 1.0f);

    } else if (timeSinceLastBeat < (att + dec)) {  // Decay
      enveloppeVolume = (timeSinceLastBeat - att) * (sus - 1.0f) / dec + 1.0f;
      enveloppeVolume = juce::jmax(sus, enveloppeVolume);

    } else if (timeSinceLastBeat >= (att + dec)) {  // Sustain
      enveloppeVolume = sus;
    }
  } else {  // Release
    enveloppeVolume = sus * (duration - timeSinceLastBeat) / rel + sus;
    enveloppeVolume = juce::jmax(0.0f, enveloppeVolume);
  }

  return enveloppeVolume;
}