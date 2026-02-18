#include "audio/audio-clip.hpp"

#include <juce_audio_utils/juce_audio_utils.h>

#include "audio/audio-context.hpp"

AudioClip::AudioClip()
    : id(juce::Uuid().toDashedString().toStdString()),
      gain(0.4f),
      position(0.0),
      duration(0.0),
      offset(0.0) {
  formatManager.registerBasicFormats();
}

AudioClip::~AudioClip() = default;

void AudioClip::loadAudioFile() {
  const juce::File file("~/daw/projects/test.dawproj/audio/" + fileName);

  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(file));
  if (reader == nullptr) return;

  auto const& ctx = AudioContext::getInstance();
  double ratio = ctx.sampleRate / reader->sampleRate;
  int resampledLength = (int)(reader->lengthInSamples * ratio);
  int numChannels = (int)reader->numChannels;

  // Read the entire file
  juce::AudioBuffer<float> fileBuffer(numChannels,
                                      (int)reader->lengthInSamples);
  reader->read(&fileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

  // Resample to engine sample rate
  audioData.setSize(numChannels, resampledLength);

  if (reader->sampleRate == ctx.sampleRate) {
    // Same rate — direct copy
    audioData = std::move(fileBuffer);
  } else {
    for (int ch = 0; ch < numChannels; ++ch) {
      juce::LagrangeInterpolator interpolator;
      interpolator.process(1.0 / ratio, fileBuffer.getReadPointer(ch),
                           audioData.getWritePointer(ch), resampledLength);
    }
  }

  loaded = true;
}

void AudioClip::renderBlock(juce::AudioBuffer<float>& buffer, int startSample,
                            int numSamples, double startTime) {
  if (!loaded) return;
  auto const& ctx = AudioContext::getInstance();

  double sampleRate = ctx.sampleRate;
  double blockDuration = (double)numSamples / sampleRate;

  double clipStart = position;
  double clipEnd = position + duration;

  double blockStart = startTime;
  double blockEnd = startTime + blockDuration;

  if (blockEnd <= clipStart || blockStart >= clipEnd) return;

  double overlapStart = std::max(blockStart, clipStart);
  double overlapEnd = std::min(blockEnd, clipEnd);

  int bufferOffset = std::max(0, (int)std::lround((overlapStart - blockStart) * sampleRate));
  int samplesToRead = (int)std::lround((overlapEnd - overlapStart) * sampleRate);

  // Position in the pre-loaded buffer (already at engine sample rate)
  int sourcePos = std::max(0, (int)std::lround((offset + (overlapStart - clipStart)) * sampleRate));

  // Clamp to avoid reading past buffer boundaries
  samplesToRead = std::min(samplesToRead, numSamples - bufferOffset);
  samplesToRead = std::min(samplesToRead, audioData.getNumSamples() - sourcePos);
  if (samplesToRead <= 0) return;

  // Copy from pre-loaded buffer to output
  int numChannels =
      std::min(buffer.getNumChannels(), audioData.getNumChannels());
  for (int ch = 0; ch < numChannels; ++ch) {
    buffer.addFrom(ch, startSample + bufferOffset, audioData, ch, sourcePos,
                   samplesToRead, gain);
  }
}

void AudioClip::setGain(float newGain) {
  this->gain = juce::jlimit(0.0f, 1.0f, newGain);
}

nlohmann::json AudioClip::toJson() const {
  nlohmann::json j;
  j["type"] = getClipType();
  j["id"] = id;
  j["name"] = name;
  j["fileName"] = fileName;
  j["gain"] = gain;
  j["position"] = position;
  j["duration"] = duration;
  j["offset"] = offset;
  return j;
}

std::unique_ptr<AudioClip> AudioClip::fromJson(const nlohmann::json& j) {
  auto clip = std::make_unique<AudioClip>();

  clip->id = j["id"].get<std::string>();
  clip->name = j["name"].get<std::string>();
  clip->fileName = j["fileName"].get<std::string>();
  clip->gain = j.value("gain", 0.4f);
  clip->position = j.value("position", 0.0f);
  clip->duration = j.value("duration", 0.0f);
  clip->offset = j.value("offset", 0.0f);

  if (!clip->fileName.empty()) {
    clip->loadAudioFile();
  }

  return clip;
}