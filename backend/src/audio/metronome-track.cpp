#include "audio/metronome-track.hpp"

#include <juce_audio_utils/juce_audio_utils.h>

#include "audio/audio-context.hpp"

WaveTable MetronomeTrack::waveTable(WaveTable::WaveType::SINE, 2048);

MetronomeTrack::MetronomeTrack(float frequency)
    : AudioTrack(), frequency(frequency), duration(0.05f) {
  att = 0.01f;
  dec = 0.02f;
  sus = 0.2f;
  rel = 0.02f;
  name = "Metronome";
}

MetronomeTrack::~MetronomeTrack() = default;

float MetronomeTrack::getSampleValue(double sampleTime, float tempo) {
  if (mute) {
    return 0.0f;
  }

  float pi = juce::MathConstants<float>::pi;
  interval = 60.0f / tempo;

  if (float timeSinceLastBeat = std::fmod(sampleTime, interval);
      timeSinceLastBeat < duration + rel) {
    float envelopeVolume = computeEnvelope(timeSinceLastBeat);
    float currentPhase = 2.0f * pi * frequency * timeSinceLastBeat;
    return envelopeVolume * getLinearGain() * waveTable.getSampleFast(currentPhase);
  }

  return 0.0f;
}

void MetronomeTrack::renderBlock(juce::AudioBuffer<float>& buffer,
                                 int startSample, int numSamples,
                                 double startTime, float tempo) {
  if (mute) {
    buffer.clear(0, startSample, numSamples);
    return;
  }

  auto const& ctx = AudioContext::getInstance();
  const float pi = juce::MathConstants<float>::pi;
  const double sampleRate = ctx.sampleRate;
  interval = 60.0f / tempo;

  float* bufferData = buffer.getWritePointer(0, startSample);

  for (int i = 0; i < numSamples; ++i) {
    const double sampleTime = startTime + (double)i / sampleRate;
    const float timeSinceLastBeat = std::fmod(sampleTime, interval);

    if (timeSinceLastBeat < duration + rel) {
      const float envelopeVolume = computeEnvelope(timeSinceLastBeat);
      const float currentPhase = 2.0f * pi * frequency * timeSinceLastBeat;
      bufferData[i] =
          envelopeVolume * getLinearGain() * waveTable.getSampleFast(currentPhase);
    } else {
      bufferData[i] = 0.0f;
    }
  }

  for (int ch = 1; ch < buffer.getNumChannels(); ++ch) {
    buffer.copyFrom(ch, startSample, buffer, 0, startSample, numSamples);
  }
}

float MetronomeTrack::computeEnvelope(float timeSinceLastBeat) const {
  float envelopeVolume = 0.0f;

  if (timeSinceLastBeat < duration) {
    if (timeSinceLastBeat < att) {
      envelopeVolume = timeSinceLastBeat / att;
      envelopeVolume = juce::jmin(envelopeVolume, 1.0f);
    } else if (timeSinceLastBeat < (att + dec)) {
      envelopeVolume = (timeSinceLastBeat - att) * (sus - 1.0f) / dec + 1.0f;
      envelopeVolume = juce::jmax(sus, envelopeVolume);
    } else {
      envelopeVolume = sus;
    }
  } else {
    envelopeVolume = sus * (1.0f - (timeSinceLastBeat - duration) / rel);
    envelopeVolume = juce::jmax(0.0f, envelopeVolume);
  }

  return envelopeVolume;
}

void MetronomeTrack::freeze(float tempo, double sampleRate) {
  const int periodSamples =
      static_cast<int>(std::ceil((60.0 / tempo) * sampleRate));

  juce::AudioBuffer<float> tempBuffer(2, periodSamples);
  tempBuffer.clear();
  renderBlock(tempBuffer, 0, periodSamples, 0.0, tempo);

  frozenPeriodSamples = periodSamples;
  frozenBuffer = std::move(tempBuffer);
  frozen.store(true, std::memory_order_release);
}

nlohmann::json MetronomeTrack::toJson() const {
  nlohmann::json j;
  j["frequency"] = frequency;
  j["volume"] = volume;
  j["mute"] = mute;
  return j;
}

std::unique_ptr<MetronomeTrack> MetronomeTrack::fromJson(
    const nlohmann::json& j) {
  float freq = j.value("frequency", 1000.0f);
  auto track = std::make_unique<MetronomeTrack>(freq);
  track->volume = j.value("volume", 0.0f);
  track->mute = j.value("mute", false);
  return track;
}
