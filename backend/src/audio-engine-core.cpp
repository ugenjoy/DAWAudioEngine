#include "audio-engine-core.hpp"
#include "audio-context.hpp"

// TODO: [HIGH] Implement dynamic track management API (addTrack, removeTrack,
// getTracks)
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

  // Pre-allocate mix buffer to avoid allocations in audio thread
  // Allocate for 2 channels (stereo output)
  mixBuffer.setSize(2, samplesPerBlockExpected, false, true, false);

  // Initialize pan values for each track (center = 0.5)
  trackPanValues.resize(tracks.size(), 0.5f);

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
  auto const& ctx = AudioContext::getInstance();
  auto* buffer = bufferToFill.buffer;
  auto numSamples = bufferToFill.numSamples;

  if (!playing) {
    buffer->clear();
    return;
  }

  // Clear the pre-allocated mix buffer
  mixBuffer.clear();

  // Mix all tracks into the buffer
  // TODO: [HIGH] Optimize track rendering - consider:
  // 1. Batch processing: tracks render entire buffer at once instead of per-sample
  // 2. SIMD operations for mixing
  // 3. Lock-free track list access
  for (int sample = 0; sample < numSamples; ++sample) {
    double sampleTime = currentPosition + (double)sample / ctx.sampleRate;
    float mixedSample = 0.0f;

    // TODO: [MEDIUM] Add track solo/mute logic here
    for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
      mixedSample += tracks[trackIdx]->getSampleValue(sampleTime);
    }

    // Apply master volume and store in mix buffer (both channels)
    mixedSample *= masterVolume;
    for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel) {
      mixBuffer.setSample(channel, sample, mixedSample);
    }
  }

  // Copy from mix buffer to output buffer
  for (int channel = 0; channel < buffer->getNumChannels(); ++channel) {
    buffer->copyFrom(channel, bufferToFill.startSample, mixBuffer,
                     channel, 0, numSamples);
  }

  // Update playback position
  // TODO: [MEDIUM] Replace floating-point accumulation with integer sample
  // counter to avoid drift: totalSampleCount += numSamples;
  currentPosition += (double)numSamples / ctx.sampleRate;
}

void AudioEngineCore::releaseResources() {
  juce::Logger::writeToLog("Releasing audio resources");
}