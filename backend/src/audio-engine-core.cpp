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

  // Pre-allocate buffers to avoid allocations in audio thread
  // Allocate for 2 channels (stereo output)
  mixBuffer.setSize(2, samplesPerBlockExpected, false, true, false);

  // Allocate mono track buffer for individual track rendering
  trackBuffer.setSize(1, samplesPerBlockExpected, false, true, false);

  // Initialize pan values for each track (center = 0.5)
  trackPanValues.resize(tracks.size(), 0.5f);

  juce::Logger::writeToLog("Audio initialized:");
  juce::Logger::writeToLog(
      "- Buffer size: " + juce::String(samplesPerBlockExpected) + " samples");
  juce::Logger::writeToLog("- Sample rate: " + juce::String(sampleRate) +
                           " Hz");
  juce::Logger::writeToLog("- Ready to play!");
}

void AudioEngineCore::play() {
  if (!playing) {
    playing.store(true);
  }
}

void AudioEngineCore::stop() {
  if (playing) {
    playing.store(false);
    currentPosition = 0.0;
  }
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

  // OPTIMIZED: Batch processing with reduced virtual calls and SIMD-enabled
  // mixing Render each track into trackBuffer, then mix into stereo mixBuffer
  for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
    // Render entire block at once (single virtual call instead of numSamples
    // calls)
    tracks[trackIdx]->renderBlock(trackBuffer, 0, numSamples, currentPosition);

    // Mix track buffer into both stereo channels using JUCE's optimized addFrom
    // This uses SIMD operations internally for much better performance
    for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel) {
      mixBuffer.addFrom(channel, 0, trackBuffer, 0, 0, numSamples);
    }
  }

  // Apply master volume to mixed buffer using SIMD-optimized operation
  for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel) {
    mixBuffer.applyGain(channel, 0, numSamples, masterVolume);
  }

  // Copy from mix buffer to output buffer
  for (int channel = 0; channel < buffer->getNumChannels(); ++channel) {
    buffer->copyFrom(channel, bufferToFill.startSample, mixBuffer, channel, 0,
                     numSamples);
  }

  // Update playback position
  // TODO: [MEDIUM] Replace floating-point accumulation with integer sample
  // counter to avoid drift: totalSampleCount += numSamples;
  currentPosition += (double)numSamples / ctx.sampleRate;
}

void AudioEngineCore::releaseResources() {
  juce::Logger::writeToLog("Releasing audio resources");
}