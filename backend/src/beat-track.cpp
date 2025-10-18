#include "beat-track.hpp"
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio-context.hpp"

BeatTrack::BeatTrack(float frequency) : AudioTrack(), frequency(frequency) {
  auto& ctx = AudioContext::getInstance();

  // Beat
  phase = 0.0f;
  interval = 60.0f / ctx.tempoBPM;
  duration = 0.2f;

  // Enveloppe
  att = 0.02f;
  dec = 0.02f;
  sus = 1.0f;
  rel = 0.02f;
}

BeatTrack::~BeatTrack() {}

float BeatTrack::getSampleValue(double sampleTime) {
  if (!mute) {
    auto& ctx = AudioContext::getInstance();
    float pi = juce::MathConstants<float>::pi;
    float timeSinceLastBeat = std::fmod(sampleTime, interval);
    float enveloppeVolume = computeEnveloppe(timeSinceLastBeat);
    float sampleValue = enveloppeVolume * volume * std::sin(phase);

    // Follow the phase
    phase += 2.0f * pi * frequency / (float)ctx.sampleRate;
    if (phase >= 2.0f * pi)
      phase -= 2.0f * pi;

    return sampleValue;
  } else {
    return 0.0f;
  }
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