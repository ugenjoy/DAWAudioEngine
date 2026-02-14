#include "audio/audio-file-track.hpp"
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/audio-context.hpp"

AudioFileTrack::AudioFileTrack() : AudioTrack() {}

AudioFileTrack::~AudioFileTrack() = default;

float AudioFileTrack::getSampleValue(double sampleTime, float tempo) {
  if (mute) {
    return 0.0f;
  }

  return 0.0f;
}

void AudioFileTrack::renderBlock(juce::AudioBuffer<float>& buffer,
                                 int startSample,
                                 int numSamples,
                                 double startTime,
                                 float tempo) {
  // Early exit if muted
  if (mute) {
    buffer.clear(0, startSample, numSamples);
    return;
  }
}

nlohmann::json AudioFileTrack::toJson() const {
  nlohmann::json j;
  j["type"] = getTrackType();
  j["id"] = id;
  j["volume"] = volume;
  j["pan"] = pan;
  j["mute"] = mute;
  return j;
}

std::unique_ptr<AudioFileTrack> AudioFileTrack::fromJson(
    const nlohmann::json& j) {
  auto track = std::make_unique<AudioFileTrack>();

  // Restore ID if present, otherwise keep the auto-generated one
  if (j.contains("id")) {
    track->id = j["id"].get<std::string>();
  }

  track->volume = j.value("volume", 0.4f);
  track->pan = j.value("pan", 0.0f);
  track->mute = j.value("mute", false);

  return track;
}