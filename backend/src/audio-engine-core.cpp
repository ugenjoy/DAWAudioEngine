#include "audio-engine-core.hpp"
#include "audio-context.hpp"

AudioEngineCore::AudioEngineCore()
    : playing(false), currentPosition(0.0), masterVolume(0.5f) {
  tracks.push_back(std::make_unique<BeatTrack>(500.0f));

  // Audio configuration : 0 inputs, 2 outputs
  setAudioChannels(0, 2);
}

AudioEngineCore::~AudioEngineCore() {
  shutdownAudio();
}

void AudioEngineCore::prepareToPlay(int samplesPerBlockExpected,
                                    double sampleRate) {
  auto& ctx = AudioContext::getInstance();
  ctx.sampleRate = sampleRate;

  juce::Logger::writeToLog("Audio initialized:");
  juce::Logger::writeToLog(
      "- Buffer size: " + juce::String(samplesPerBlockExpected) + " samples");
  juce::Logger::writeToLog("- Sample rate: " + juce::String(sampleRate) +
                           " Hz");
  juce::Logger::writeToLog("- Ready to play !");

  // Start automatically (for testing)
  playing = true;
}

void AudioEngineCore::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill) {
  auto& ctx = AudioContext::getInstance();
  auto* buffer = bufferToFill.buffer;
  auto numSamples = bufferToFill.numSamples;
  float pi = juce::MathConstants<float>::pi;

  if (!playing) {
    buffer->clear();
    return;
  }

  for (int sample = 0; sample < numSamples; ++sample) {
    double sampleTime = currentPosition + (double)sample / ctx.sampleRate;
    float sampleValue = 0.0f;

    for (int i = 0; i < tracks.size(); i++) {
      sampleValue += tracks[i]->getSampleValue(sampleTime) * masterVolume;
    }

    // Apply on the 2 output channels
    for (int channel = 0; channel < buffer->getNumChannels(); ++channel) {
      buffer->setSample(channel, sample, sampleValue);
    }
  }

  // Update time
  currentPosition += (double)numSamples / ctx.sampleRate;
}

void AudioEngineCore::releaseResources() {
  juce::Logger::writeToLog("Free audio resources");
}