#include "audio/audio-file-track.hpp"

#include <juce_audio_utils/juce_audio_utils.h>

#include "audio/audio-context.hpp"

AudioFileTrack::AudioFileTrack()
    : AudioTrack(), clipsManager(std::make_unique<ClipsManager>()) {}

AudioFileTrack::~AudioFileTrack() = default;

float AudioFileTrack::getSampleValue(double sampleTime, float tempo) {
  if (mute) {
    return 0.0f;
  }

  return 0.0f;
}

void AudioFileTrack::renderBlock(juce::AudioBuffer<float>& buffer,
                                 int startSample, int numSamples,
                                 double startTime, float tempo) {
  // Early exit if muted
  if (mute) {
    buffer.clear(0, startSample, numSamples);
    return;
  }

  clipsManager->renderClips(buffer, numSamples, startTime);

  // Apply track volume
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    buffer.applyGain(ch, startSample, numSamples, getLinearGain());
  }
}

void AudioFileTrack::sampleRateChanged() {
  clipsManager->sampleRateChanged();
}

nlohmann::json AudioFileTrack::toJson() const {
  nlohmann::json j;
  j["type"] = getTrackType();
  j["id"] = id;
  j["name"] = name;
  j["volume"] = volume;
  j["pan"] = pan;
  j["mute"] = mute;
  j["solo"] = solo;
  j["clips"] = clipsManager->toJson();
  j["inputChannel"] = inputChannel.load(std::memory_order_relaxed);
  j["inputStereo"] = inputStereo.load(std::memory_order_relaxed);
  j["monitoring"] = monitoring.load(std::memory_order_relaxed);
  j["color"] = color;
  return j;
}

std::unique_ptr<AudioFileTrack> AudioFileTrack::fromJson(
    const nlohmann::json& j) {
  auto track = std::make_unique<AudioFileTrack>();

  // Restore ID if present, otherwise keep the auto-generated one
  if (j.contains("id")) {
    track->id = j["id"].get<std::string>();
  }

  track->name = j["name"].get<std::string>();
  track->volume = j.value("volume", 0.0f);
  track->pan = j.value("pan", 0.0f);
  track->mute = j.value("mute", false);
  track->solo = j.value("solo", false);
  track->inputChannel.store(j.value("inputChannel", -1), std::memory_order_relaxed);
  track->inputStereo.store(j.value("inputStereo", false), std::memory_order_relaxed);
  track->monitoring.store(j.value("monitoring", false), std::memory_order_relaxed);
  track->color = j.value("color", track->color);

  // Load clips
  if (j.contains("clips")) {
    track->clipsManager->loadFromJson(j["clips"]);
  }

  return track;
}