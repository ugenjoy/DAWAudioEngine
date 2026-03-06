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

void AudioClip::loadAudioFile(const std::string& audioDir) {
  const juce::File file(audioDir.empty()
      ? ("~/daw/projects/test.dawproj/audio/" + fileName)
      : (audioDir + "/" + fileName));

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

  // Clamp duration to actual audio file length (accounting for offset)
  double audioDuration = resampledLength / ctx.sampleRate;
  double maxDuration = std::max(0.0, audioDuration - offset);
  if (duration <= 0.0 || duration > maxDuration) {
    duration = maxDuration;
  }

  generateWaveformPeaks();
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

void AudioClip::generateWaveformPeaks(int pointsPerSecond) {
  if (!loaded || audioData.getNumSamples() == 0 || duration <= 0.0) return;

  auto const& ctx = AudioContext::getInstance();
  int samplesPerPoint = (int)(ctx.sampleRate / pointsPerSecond);
  int totalSamples = audioData.getNumSamples();

  // Generate peaks for the visible portion [offset, offset + duration]
  // duration is guaranteed to be clamped to the audio file length at load time
  int startSample = std::max(0, (int)std::lround(offset * ctx.sampleRate));
  int endSample =
      std::min(totalSamples, (int)std::lround((offset + duration) * ctx.sampleRate));

  int visibleSamples = endSample - startSample;
  if (visibleSamples <= 0) return;

  int numPoints = (visibleSamples + samplesPerPoint - 1) / samplesPerPoint;

  waveformPeaks.clear();
  waveformPeaks.reserve(numPoints * 2);

  for (int i = 0; i < numPoints; ++i) {
    int start = startSample + i * samplesPerPoint;
    int end = std::min(start + samplesPerPoint, endSample);

    float minVal = 1.0f;
    float maxVal = -1.0f;

    for (int ch = 0; ch < audioData.getNumChannels(); ++ch) {
      const float* data = audioData.getReadPointer(ch);
      for (int s = start; s < end; ++s) {
        float sample = data[s];
        if (sample < minVal) minVal = sample;
        if (sample > maxVal) maxVal = sample;
      }
    }

    waveformPeaks.push_back(minVal);
    waveformPeaks.push_back(maxVal);
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
  j["waveform"] = waveformPeaks;
  return j;
}

std::unique_ptr<AudioClip> AudioClip::fromJson(const nlohmann::json& j,
                                                const std::string& audioDir) {
  auto clip = std::make_unique<AudioClip>();

  clip->id = j["id"].get<std::string>();
  clip->name = j["name"].get<std::string>();
  clip->fileName = j["fileName"].get<std::string>();
  clip->gain = j.value("gain", 0.4f);
  clip->position = j.value("position", 0.0f);
  clip->duration = j.value("duration", 0.0f);
  clip->offset = j.value("offset", 0.0f);

  if (!clip->fileName.empty()) {
    clip->loadAudioFile(audioDir);
  }

  return clip;
}