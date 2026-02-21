#include "audio/audio-engine-core.hpp"

#include "audio/audio-context.hpp"

// TODO: [MEDIUM] Add audio mixer with bus routing and effects chain
// TODO: [MEDIUM] Implement error handling for audio device failures
// TODO: [LOW] Add panning control per track

AudioEngineCore::AudioEngineCore()
    : playing(false),
      playheadPosition(0.0),
      cursorPosition(0.0),
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
  playheadPosition.store(0, std::memory_order_relaxed);
  cursorPosition.store(0, std::memory_order_relaxed);
  juce::Logger::writeToLog("[AudioEngine] song loaded");

  if (wsServer != nullptr) {
    // Broadcast project loaded event to all clients
    nlohmann::json loadedSongMsg;
    loadedSongMsg["type"] = "broadcast";
    loadedSongMsg["event"] = "song.loaded";
    loadedSongMsg["song"] = activeSong->toJson();
    wsServer->broadcast(loadedSongMsg.dump());

    // Broadcast playhead position to all clients
    nlohmann::json posMsg;
    posMsg["type"] = "broadcast";
    posMsg["event"] = "transport.playheadpPosition";
    posMsg["position"] = playheadPosition.load(std::memory_order_relaxed);
    wsServer->broadcast(posMsg.dump());

    // Broadcast cursor position to all clients
    nlohmann::json startPosMsg;
    startPosMsg["type"] = "broadcast";
    startPosMsg["event"] = "transport.cursorPosition";
    startPosMsg["position"] = cursorPosition.load(std::memory_order_relaxed);
    wsServer->broadcast(startPosMsg.dump());
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
      playheadPosition.store(cursorPosition, std::memory_order_relaxed);
    } else {
      cursorPosition.store(0.0, std::memory_order_relaxed);
      playheadPosition.store(0.0, std::memory_order_relaxed);
    }

    if (wsServer != nullptr) {
      // Broadcast transport stop event to all clients
      nlohmann::json stopMsg;
      stopMsg["type"] = "broadcast";
      stopMsg["event"] = "transport.stop";
      wsServer->broadcast(stopMsg.dump());

      // Broadcast playhead position event to all clients
      nlohmann::json playheadPositionMsg;
      playheadPositionMsg["type"] = "broadcast";
      playheadPositionMsg["event"] = "transport.playheadPosition";
      playheadPositionMsg["position"] =
          playheadPosition.load(std::memory_order_relaxed);
      wsServer->broadcast(playheadPositionMsg.dump());

      // Broadcast cursor position event to all clients
      nlohmann::json cursorPositionMsg;
      cursorPositionMsg["type"] = "broadcast";
      cursorPositionMsg["event"] = "transport.cursorPosition";
      cursorPositionMsg["position"] =
          cursorPosition.load(std::memory_order_relaxed);
      wsServer->broadcast(cursorPositionMsg.dump());
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

void AudioEngineCore::setPlayheadPosition(double position) {
  if (activeSong) {
    playheadPosition.store(position, std::memory_order_relaxed);

    if (wsServer != nullptr) {
      // Broadcast playhead position event to all clients
      nlohmann::json positionMsg;
      positionMsg["type"] = "broadcast";
      positionMsg["event"] = "transport.playheadPosition";
      positionMsg["position"] =
          playheadPosition.load(std::memory_order_relaxed);
      wsServer->broadcast(positionMsg.dump());
    }
  }
}

void AudioEngineCore::setCursorPosition(double position) {
  if (activeSong) {
    if (!playing) {
      playheadPosition.store(position, std::memory_order_relaxed);
    }
    cursorPosition.store(position, std::memory_order_relaxed);

    if (wsServer != nullptr) {
      // Broadcast cursor position event to all clients
      nlohmann::json cursorPositionMsg;
      cursorPositionMsg["type"] = "broadcast";
      cursorPositionMsg["event"] = "transport.cursorPosition";
      cursorPositionMsg["position"] =
          cursorPosition.load(std::memory_order_relaxed);
      wsServer->broadcast(cursorPositionMsg.dump());

      // Broadcast playhead position event to all clients
      nlohmann::json playheadPositionMsg;
      playheadPositionMsg["type"] = "broadcast";
      playheadPositionMsg["event"] = "transport.playheadPosition";
      playheadPositionMsg["position"] =
          playheadPosition.load(std::memory_order_relaxed);
      wsServer->broadcast(playheadPositionMsg.dump());
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
                     playheadPosition.load(std::memory_order_relaxed));

  auto const& ctx = AudioContext::getInstance();

  playheadPosition.store(playheadPosition + (double)numSamples / ctx.sampleRate,
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
  msg["event"] = "transport.playheadPosition";
  msg["position"] = playheadPosition.load(std::memory_order_relaxed);

  wsServer->broadcast(msg.dump());
}

void AudioEngineCore::releaseResources() {
  juce::Logger::writeToLog("Releasing audio resources");
}