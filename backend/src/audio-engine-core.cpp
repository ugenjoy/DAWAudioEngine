#include "audio-engine-core.hpp"
#include "audio-context.hpp"

// TODO: [CRITICAL] Pre-allocate audio buffers to avoid allocations in audio thread
// TODO: [HIGH] Implement dynamic track management API (addTrack, removeTrack, getTracks)
// TODO: [MEDIUM] Add audio mixer with bus routing and effects chain
// TODO: [MEDIUM] Implement error handling for audio device failures
// TODO: [LOW] Add panning control per track

AudioEngineCore::AudioEngineCore()
    : playing(false), currentPosition(0.0), masterVolume(0.5f) {
  // TODO: [HIGH] Replace hardcoded track with dynamic track management
  // Suggestion: loadTracksFromConfig() or addTrack() API
  tracks.push_back(std::make_unique<BeatTrack>(1000.0f));

  // Audio configuration: 0 inputs, 2 outputs
  // TODO: [MEDIUM] Add error handling for audio device initialization
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
  juce::Logger::writeToLog("- Ready to play!");

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

  // TODO: [CRITICAL] PERFORMANCE - This loop has several issues:
  // 1. Avoid per-sample time calculation (use sample counter instead)
  // 2. Pre-allocate mix buffer to avoid repeated additions
  // 3. Use SIMD for mixing multiple tracks
  // 4. Consider lock-free communication for track list access
  
  // TODO: [HIGH] Replace with:
  // int64_t sampleOffset = 0;
  // for (int sample = 0; sample < numSamples; ++sample) {
  //   double sampleTime = (totalSampleCount + sampleOffset++) / ctx.sampleRate;
  //   ...
  // }
  
  for (int sample = 0; sample < numSamples; ++sample) {
    double sampleTime = currentPosition + (double)sample / ctx.sampleRate;
    float sampleValue = 0.0f;

    // TODO: [MEDIUM] Add track solo/mute logic here
    for (int i = 0; i < tracks.size(); i++) {
      sampleValue += tracks[i]->getSampleValue(sampleTime) * masterVolume;
    }

    // Apply to both output channels
    for (int channel = 0; channel < buffer->getNumChannels(); ++channel) {
      buffer->setSample(channel, sample, sampleValue);
    }
  }

  // Update playback position
  // TODO: [MEDIUM] Replace floating-point accumulation with integer sample counter
  // to avoid drift: totalSampleCount += numSamples;
  currentPosition += (double)numSamples / ctx.sampleRate;
}

void AudioEngineCore::releaseResources() {
  juce::Logger::writeToLog("Releasing audio resources");
}