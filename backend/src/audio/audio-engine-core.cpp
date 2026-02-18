#include "audio/audio-engine-core.hpp"

#include "audio/audio-context.hpp"

// TODO: [MEDIUM] Add audio mixer with bus routing and effects chain
// TODO: [MEDIUM] Implement error handling for audio device failures
// TODO: [LOW] Add panning control per track

AudioEngineCore::AudioEngineCore()
    : playing(false),
      currentPosition(0.0),
      masterVolume(0.5f),
      activeSong(nullptr) {
  // Audio configuration: 0 inputs, 2 outputs
  // TODO: [MEDIUM] Add error handling for audio device initialization
  setAudioChannels(0, 2);
}

AudioEngineCore::~AudioEngineCore() {
  stopTimer();
  shutdownAudio();
}

void AudioEngineCore::prepareToPlay(int samplesPerBlockExpected,
                                    double sampleRate) {
  auto& ctx = AudioContext::getInstance();
  ctx.sampleRate = sampleRate;

  // Pre-allocate buffers to avoid allocations in audio thread
  // Allocate for 2 channels (stereo output)
  mixBuffer.setSize(2, samplesPerBlockExpected, false, true, false);

  // Allocate stereo track buffer for individual track rendering
  trackBuffer.setSize(2, samplesPerBlockExpected, false, true, false);

  juce::Logger::writeToLog("Audio initialized:");
  juce::Logger::writeToLog(
      "- Buffer size: " + juce::String(samplesPerBlockExpected) + " samples");
  juce::Logger::writeToLog("- Sample rate: " + juce::String(sampleRate) +
                           " Hz");
  juce::Logger::writeToLog("- Ready to play!");
}

void AudioEngineCore::loadSong(Song* newSong) {
  activeSong = newSong;
  currentPosition.store(0, std::memory_order_relaxed);
  juce::Logger::writeToLog("[AudioEngine] song loaded");

  if (wsServer != nullptr) {
    // Broadcast project loaded event to all clients
    nlohmann::json loadedSongMsg;
    loadedSongMsg["type"] = "broadcast";
    loadedSongMsg["event"] = "song.loaded";
    loadedSongMsg["song"] = activeSong->toJson();
    wsServer->broadcast(loadedSongMsg.dump());

    // Broadcast transport position to all clients
    nlohmann::json posMsg;
    loadedSongMsg["type"] = "broadcast";
    loadedSongMsg["event"] = "transport.position";
    loadedSongMsg["position"] = currentPosition.load(std::memory_order_relaxed);
    wsServer->broadcast(loadedSongMsg.dump());
  }
}

void AudioEngineCore::play() {
  if (activeSong && !playing) {
    playing.store(true);

    startTimerHz(30);

    if (wsServer != nullptr) {
      // Broadcast transport play event to all clients
      nlohmann::json broadcast;
      broadcast["type"] = "broadcast";
      broadcast["event"] = "transport.play";

      wsServer->broadcast(broadcast.dump());
    }
  }
}

void AudioEngineCore::pause() {
  if (activeSong && playing) {
    playing.store(false);

    stopTimer();

    if (wsServer != nullptr) {
      // Broadcast transport pause event to all clients
      nlohmann::json broadcast;
      broadcast["type"] = "broadcast";
      broadcast["event"] = "transport.pause";

      wsServer->broadcast(broadcast.dump());
    }
  }
}

void AudioEngineCore::stop() {
  if (activeSong) {
    if (playing) {
      playing.store(false);
      stopTimer();
    }
    currentPosition.store(0.0, std::memory_order_relaxed);

    if (wsServer != nullptr) {
      // Broadcast transport stop event to all clients
      nlohmann::json stopMsg;
      stopMsg["type"] = "broadcast";
      stopMsg["event"] = "transport.stop";
      wsServer->broadcast(stopMsg.dump());

      // Broadcast transport position event to all clients
      nlohmann::json positionMsg;
      positionMsg["type"] = "broadcast";
      positionMsg["event"] = "transport.position";
      positionMsg["position"] = currentPosition.load(std::memory_order_relaxed);
      wsServer->broadcast(positionMsg.dump());
    }
  }
}

void AudioEngineCore::switchPlaying() {
  if (activeSong) {
    bool wasPlaying = playing.load();
    playing.store(!wasPlaying);

    if (!wasPlaying) {
      startTimerHz(30);
    } else {
      stopTimer();
    }
  }
}

void AudioEngineCore::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill) {
  auto* buffer = bufferToFill.buffer;
  auto numSamples = bufferToFill.numSamples;

  if (!playing || !activeSong) {
    buffer->clear();
    return;
  }

  // Clear the pre-allocated mix buffer
  mixBuffer.clear();

  // Render song
  activeSong->render(mixBuffer, trackBuffer, numSamples,
                     currentPosition.load(std::memory_order_relaxed));

  auto const& ctx = AudioContext::getInstance();

  currentPosition.store(currentPosition + (double)numSamples / ctx.sampleRate,
                        std::memory_order_relaxed);

  // Apply master volume to mixed buffer using SIMD-optimized operation
  for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel) {
    mixBuffer.applyGain(channel, 0, numSamples, masterVolume);
  }

  // Copy from mix buffer to output buffer
  for (int channel = 0; channel < buffer->getNumChannels(); ++channel) {
    buffer->copyFrom(channel, bufferToFill.startSample, mixBuffer, channel, 0,
                     numSamples);
  }
}

void AudioEngineCore::timerCallback() {
  if (wsServer == nullptr || activeSong == nullptr) return;

  nlohmann::json msg;
  msg["type"] = "broadcast";
  msg["event"] = "transport.position";
  msg["position"] = currentPosition.load(std::memory_order_relaxed);

  wsServer->broadcast(msg.dump());
}

void AudioEngineCore::releaseResources() {
  juce::Logger::writeToLog("Releasing audio resources");
}