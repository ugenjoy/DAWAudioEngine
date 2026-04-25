#include "audio/audio-engine-core.hpp"

#include "audio/audio-context.hpp"
#include "events/event-engine.hpp"
#include "websocket/broadcast-helpers.hpp"
#include "websocket/websocket-server.hpp"

AudioEngineCore::AudioEngineCore()
    : playing(false),
      playheadPosition(0.0),
      cursorPosition(0.0),
      activeSong(nullptr) {
  auto savedState = loadSettings();

  auto result = deviceManager.initialise(
      64,                // numInputChannelsNeeded
      2,                 // numOutputChannelsNeeded
      savedState.get(),  // savedState (XML) — nullptr if no saved settings
      true               // selectDefaultDeviceOnFailure
  );

  if (result.isNotEmpty()) {
    juce::Logger::writeToLog("Audio device error: " + result);
  }

  deviceManager.addAudioCallback(this);
}

AudioEngineCore::~AudioEngineCore() {
  stopTimer();
  deviceManager.removeAudioCallback(this);
  deviceManager.closeAudioDevice();
}

void AudioEngineCore::audioDeviceAboutToStart(juce::AudioIODevice* device) {
  auto sampleRate = device->getCurrentSampleRate();
  auto bufferSize = device->getCurrentBufferSizeSamples();

  auto& ctx = AudioContext::getInstance();
  ctx.sampleRate = sampleRate;

  // Pre-allocate buffers to avoid allocations in audio thread
  int numActiveInputs = device->getActiveInputChannels().countNumberOfSetBits();
  mixBuffer.setSize(2, bufferSize, false, true, false);
  trackBuffer.setSize(2, bufferSize, false, true, false);
  inputBuffer.setSize(numActiveInputs, bufferSize, false, true, false);

  // Reload audio clips if sample rate changed (resampling needed)
  if (activeSong != nullptr) {
    activeSong->sampleRateChanged();
  }

  juce::Logger::writeToLog("Audio initialized:");
  juce::Logger::writeToLog("- Buffer size: " + juce::String(bufferSize) +
                           " samples");
  juce::Logger::writeToLog("- Sample rate: " + juce::String(sampleRate) +
                           " Hz");
  juce::Logger::writeToLog(
      "- Input channels: " +
      juce::String(device->getActiveInputChannels().countNumberOfSetBits()));
  juce::Logger::writeToLog(
      "- Output channels: " +
      juce::String(device->getActiveOutputChannels().countNumberOfSetBits()));
  juce::Logger::writeToLog("- Ready to play!");
}

void AudioEngineCore::audioDeviceStopped() {
  juce::Logger::writeToLog("Audio device stopped");
}

void AudioEngineCore::unloadSong() {
  activeSong = nullptr;
  playheadPosition.store(0.0, std::memory_order_relaxed);
  cursorPosition.store(0.0, std::memory_order_relaxed);
  prevTimerPosition = 0.0;
  monitoringTrackCount.store(0, std::memory_order_relaxed);
  monitoredChannelMask.store(0, std::memory_order_relaxed);
  if (loopManager) loopManager->reset();
}

void AudioEngineCore::setEndPositionCallback(EndPositionCallback cb) {
  endPositionCallback = std::move(cb);
}

void AudioEngineCore::setLoopManager(LoopManager* lm) {
  loopManager = lm;
}

void AudioEngineCore::setEventEngine(EventEngine* engine, AppContext* ctx) {
  eventEngine = engine;
  appContext = ctx;
}

void AudioEngineCore::loadSong(Song* newSong) {
  activeSong = newSong;
  endPositionFired = false;
  if (loopManager) loopManager->reset();
  playheadPosition.store(0, std::memory_order_relaxed);
  prevTimerPosition = 0.0;
  cursorPosition.store(0, std::memory_order_relaxed);

  // Tracks loaded from JSON may already have monitoring enabled,
  // so we must rebuild the atomic counter and bitmask to match.
  int count = 0;
  for (const auto& track : activeSong->getTracksManager()->getTracks()) {
    if (track->isMonitoring()) ++count;
  }
  monitoringTrackCount.store(count, std::memory_order_relaxed);
  rebuildMonitoredChannelMask();

  juce::Logger::writeToLog("[AudioEngine] song loaded");

  if (wsServer != nullptr) {
    broadcast::send(*wsServer, "song.loaded", {{"song", activeSong->toJson()}});
    broadcast::send(*wsServer, "transport.playheadPosition",
                    {{"position", playheadPosition.load(std::memory_order_relaxed)}});
    broadcast::send(*wsServer, "transport.cursorPosition",
                    {{"position", cursorPosition.load(std::memory_order_relaxed)}});
  }
}

void AudioEngineCore::play() {
  if (activeSong && !playing) {
    // Reset so the end-position check is active for the new session, even if a
    // previous session ended and the async stop callback hasn't run yet.
    endPositionFired.store(false, std::memory_order_relaxed);
    playing.store(true);

    startTimerHz(10);

    if (wsServer != nullptr) {
      broadcast::send(*wsServer, "transport.play");
    }
  }
}

void AudioEngineCore::pause() {
  if (activeSong && playing) {
    playing.store(false);

    stopTimer();

    if (wsServer != nullptr) {
      broadcast::send(*wsServer, "transport.pause");
    }
  }
}

void AudioEngineCore::stop() {
  if (activeSong) {
    endPositionFired.store(false, std::memory_order_relaxed);
    bool hadActiveLoop = loopManager && loopManager->hasActiveLoop();
    if (loopManager) loopManager->reset();
    // Two-phase stop: first press returns to cursor position,
    // second press (already paused) resets everything to zero.
    if (playing) {
      playing.store(false);
      stopTimer();
      playheadPosition.store(cursorPosition.load(std::memory_order_relaxed),
                              std::memory_order_relaxed);
    } else {
      cursorPosition.store(0.0, std::memory_order_relaxed);
      playheadPosition.store(0.0, std::memory_order_relaxed);
    }

    prevTimerPosition = playheadPosition.load(std::memory_order_relaxed);

    if (wsServer != nullptr) {
      broadcast::send(*wsServer, "transport.stop");
      broadcast::send(*wsServer, "transport.playheadPosition",
                      {{"position", playheadPosition.load(std::memory_order_relaxed)}});
      broadcast::send(*wsServer, "transport.cursorPosition",
                      {{"position", cursorPosition.load(std::memory_order_relaxed)}});
      if (hadActiveLoop)
        broadcast::send(*wsServer, "loop.deactivated");
    }
  }
}

void AudioEngineCore::switchPlaying() {
  if (activeSong) {
    bool wasPlaying = playing.load();
    playing.store(!wasPlaying);

    if (!wasPlaying) {
      startTimerHz(60);
    } else {
      stopTimer();
    }
  }
}

void AudioEngineCore::setPlayheadPosition(double position) {
  if (activeSong) {
    playheadPosition.store(position, std::memory_order_relaxed);
    // Step back by a tiny epsilon so that any position trigger sitting exactly
    // at this position satisfies the strict prevPos < triggerPos condition on
    // the next timer tick.
    prevTimerPosition = std::max(0.0, position - 1e-6);

    if (wsServer != nullptr) {
      broadcast::send(*wsServer, "transport.playheadPosition",
                      {{"position", playheadPosition.load(std::memory_order_relaxed)}});
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
      broadcast::send(*wsServer, "transport.cursorPosition",
                      {{"position", cursorPosition.load(std::memory_order_relaxed)}});
      broadcast::send(*wsServer, "transport.playheadPosition",
                      {{"position", playheadPosition.load(std::memory_order_relaxed)}});
    }
  }
}

void AudioEngineCore::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData, int numInputChannels,
    float* const* outputChannelData, int numOutputChannels, int numSamples,
    const juce::AudioIODeviceCallbackContext& /*context*/) {
  // Bitmask of input channels needed by monitoring tracks.
  // Each bit corresponds to a hardware input channel index.
  // Only copy channels that at least one track is monitoring.
  const uint64_t mask = monitoredChannelMask.load(std::memory_order_relaxed);
  const bool hasMonitoring = mask != 0;

  if (hasMonitoring) {
    const int maxCh =
        std::min({numInputChannels, inputBuffer.getNumChannels(), 64});
    for (int ch = 0; ch < maxCh; ++ch) {
      if ((mask >> ch) & 1ULL) {
        if (inputChannelData[ch] != nullptr) {
          inputBuffer.copyFrom(ch, 0, inputChannelData[ch], numSamples);
        }
      } else {
        inputBuffer.clear(ch, 0, numSamples);
      }
    }
  } else {
    inputBuffer.clear();
  }

  if ((!playing && !hasMonitoring) || !activeSong) {
    // Clear output
    for (int ch = 0; ch < numOutputChannels; ++ch) {
      juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
    }
    return;
  }

  // Clear the pre-allocated mix buffer
  mixBuffer.clear();

  bool isPlaying = playing.load(std::memory_order_relaxed);

  // Render song with input buffer for monitoring
  activeSong->render(mixBuffer, trackBuffer, inputBuffer, numSamples,
                     playheadPosition.load(std::memory_order_relaxed),
                     isPlaying);

  if (isPlaying) {
    auto const& ctx = AudioContext::getInstance();
    const double posBeforeAdvance =
        playheadPosition.load(std::memory_order_relaxed);
    playheadPosition.store(
        posBeforeAdvance + (double)numSamples / ctx.sampleRate,
        std::memory_order_relaxed);

    bool positionOverridden = false;

    // Loop position check
    if (loopManager) {
      double curr = playheadPosition.load(std::memory_order_relaxed);
      double prev = curr - (double)numSamples / ctx.sampleRate;
      auto loopResult = loopManager->checkPosition(prev, curr);

      if (loopResult.jumpTo.has_value()) {
        playheadPosition.store(*loopResult.jumpTo, std::memory_order_relaxed);
        positionOverridden = true;
      }

      if (loopResult.activated) {
        if (wsServer != nullptr) {
          auto* ws = wsServer;
          auto loop = loopManager->getActiveLoop();
          if (loop.has_value()) {
            auto loopJson = nlohmann::json{
                {"id", loop->id}, {"start", loop->start}, {"end", loop->end}};
            juce::MessageManager::callAsync([ws, loopJson]() {
              broadcast::send(*ws, "loop.activated", {{"loop", loopJson}});
            });
          }
        }
      }
    }

    if (endPositionFired.load(std::memory_order_relaxed)) {
      // Persistent clamp: keep the playhead at end while the async stop
      // callback is pending, preventing overshoot across multiple buffers.
      // For "continue" transitions, loadSong() resets this flag and the
      // clamp stops as soon as the next song is loaded.
      auto endPos = activeSong->getEndPosition();
      if (endPos.has_value()) {
        playheadPosition.store(*endPos, std::memory_order_relaxed);
        positionOverridden = true;
      }
    } else if (endPositionCallback) {
      auto endPos = activeSong->getEndPosition();
      if (endPos.has_value() &&
          playheadPosition.load(std::memory_order_relaxed) >= *endPos) {
        endPositionFired.store(true, std::memory_order_relaxed);
        playheadPosition.store(*endPos, std::memory_order_relaxed);
        positionOverridden = true;
        auto cb = endPositionCallback;
        juce::MessageManager::callAsync([cb]() { cb(); });
      }
    }

    // Seek trigger detection — audio-accurate (one buffer latency instead of
    // one timer period). Skipped if the loop manager or end-position logic
    // already overrode the playhead position this buffer.
    if (!positionOverridden && eventEngine != nullptr && appContext != nullptr) {
      double currPos = playheadPosition.load(std::memory_order_relaxed);
      auto seekTarget = eventEngine->checkSeekTrigger(posBeforeAdvance, currPos);
      if (seekTarget.has_value()) {
        bool hadActiveLoop = loopManager && loopManager->hasActiveLoop();
        if (loopManager) loopManager->reset();
        playheadPosition.store(*seekTarget, std::memory_order_relaxed);
        // Signal timerCallback to re-sync prevTimerPosition (avoids double-fire)
        audioSeekApplied.store(*seekTarget, std::memory_order_release);
        if (wsServer != nullptr) {
          auto* ws = wsServer;
          double pos = *seekTarget;
          juce::MessageManager::callAsync([ws, pos, hadActiveLoop]() {
            if (hadActiveLoop)
              broadcast::send(*ws, "loop.deactivated");
            broadcast::send(*ws, "transport.playheadPosition",
                            {{"position", pos}});
          });
        }
      }
    }
  }

  // Apply master volume to mixed buffer
  float vol = masterVolume.load(std::memory_order_relaxed);
  for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel) {
    mixBuffer.applyGain(channel, 0, numSamples, vol);
  }

  // Copy from mix buffer to output channel pointers
  for (int ch = 0; ch < numOutputChannels; ++ch) {
    int srcCh = std::min(ch, mixBuffer.getNumChannels() - 1);
    juce::FloatVectorOperations::copy(
        outputChannelData[ch], mixBuffer.getReadPointer(srcCh), numSamples);
  }
}

void AudioEngineCore::setMasterVolume(float volume) {
  masterVolume.store(juce::jlimit(0.0f, 1.0f, volume),
                     std::memory_order_relaxed);
}

void AudioEngineCore::setMonitoringEnabled(bool enabled) {
  // Kept for API compatibility; the channel mask drives the actual behaviour.
  monitoringEnabled.store(enabled, std::memory_order_relaxed);
}

void AudioEngineCore::rebuildMonitoredChannelMask() {
  if (!activeSong) {
    monitoredChannelMask.store(0, std::memory_order_relaxed);
    return;
  }
  uint64_t mask = 0;
  for (const auto& track : activeSong->getTracksManager()->getTracks()) {
    if (track->isMonitoring()) {
      int ch = track->getInputChannel();
      if (ch >= 0 && ch < 64) {
        mask |= (1ULL << ch);
        if (track->isInputStereo() && ch + 1 < 64) {
          mask |= (1ULL << (ch + 1));
        }
      }
    }
  }
  monitoredChannelMask.store(mask, std::memory_order_relaxed);
}

void AudioEngineCore::freezeTracks() {
  if (!activeSong) return;
  auto* device = deviceManager.getCurrentAudioDevice();
  double sr = device ? device->getCurrentSampleRate()
                     : AudioContext::getInstance().sampleRate;
  activeSong->freezeAllTracks(activeSong->getTempo(), sr);
}

void AudioEngineCore::unfreezeTracks() {
  if (!activeSong) return;
  activeSong->unfreezeAllTracks();
}

void AudioEngineCore::timerCallback() {
  if (wsServer == nullptr || activeSong == nullptr) return;

  // If the audio callback applied a seek this cycle, re-sync the timer
  // baseline so firePosition doesn't re-fire the same trigger.
  double seekApplied =
      audioSeekApplied.exchange(-1.0, std::memory_order_acquire);
  if (seekApplied >= 0.0)
    prevTimerPosition = std::max(0.0, seekApplied - 1e-6);

  const double currentPos = playheadPosition.load(std::memory_order_relaxed);

  double broadcastPos = currentPos;
  if (eventEngine != nullptr && appContext != nullptr && playing.load(std::memory_order_relaxed)) {
    auto snapPos = eventEngine->firePosition(prevTimerPosition, currentPos, *appContext);
    if (snapPos.has_value()) {
      // Pause snap: setPlayheadPosition stores the exact position and broadcasts it;
      // override the final broadcast too so currentPos doesn't overwrite the snap.
      setPlayheadPosition(*snapPos);
      broadcastPos = *snapPos;
    } else if (!playing.load(std::memory_order_relaxed)) {
      // stop() fired during firePosition; use the updated playhead (cursor pos)
      // instead of the pre-stop currentPos for the final broadcast.
      broadcastPos = playheadPosition.load(std::memory_order_relaxed);
    }
  }
  prevTimerPosition = currentPos;

  broadcast::send(*wsServer, "transport.playheadPosition",
                  {{"position", broadcastPos}});

  nlohmann::json levels = nlohmann::json::object();
  for (const auto& track : activeSong->getTracksManager()->getTracks()) {
    levels[track->getId()] = track->peakLevel.load(std::memory_order_relaxed);
  }
  broadcast::send(*wsServer, "track.levels", {{"levels", levels}});
}

nlohmann::json AudioEngineCore::getAvailableDevices() {
  nlohmann::json result;
  result["deviceTypes"] = nlohmann::json::array();

  for (auto* type : deviceManager.getAvailableDeviceTypes()) {
    nlohmann::json deviceType;
    deviceType["name"] = type->getTypeName().toStdString();
    deviceType["outputDevices"] = nlohmann::json::array();
    deviceType["inputDevices"] = nlohmann::json::array();

    auto outputNames = type->getDeviceNames(false);
    for (auto& name : outputNames) {
      deviceType["outputDevices"].push_back(name.toStdString());
    }

    auto inputNames = type->getDeviceNames(true);
    for (auto& name : inputNames) {
      deviceType["inputDevices"].push_back(name.toStdString());
    }

    result["deviceTypes"].push_back(deviceType);
  }

  // Current device info
  auto* currentDevice = deviceManager.getCurrentAudioDevice();
  if (currentDevice != nullptr) {
    auto setup = deviceManager.getAudioDeviceSetup();

    nlohmann::json availableSampleRates = nlohmann::json::array();
    for (auto rate : currentDevice->getAvailableSampleRates()) {
      availableSampleRates.push_back(rate);
    }

    nlohmann::json availableBufferSizes = nlohmann::json::array();
    for (auto size : currentDevice->getAvailableBufferSizes()) {
      availableBufferSizes.push_back(size);
    }

    result["current"] = {
        {"deviceType", currentDevice->getTypeName().toStdString()},
        {"outputDevice", setup.outputDeviceName.toStdString()},
        {"inputDevice", setup.inputDeviceName.toStdString()},
        {"sampleRate", currentDevice->getCurrentSampleRate()},
        {"bufferSize", currentDevice->getCurrentBufferSizeSamples()},
        {"availableSampleRates", availableSampleRates},
        {"availableBufferSizes", availableBufferSizes}};
  }

  return result;
}

nlohmann::json AudioEngineCore::getCurrentDeviceInfo() const {
  nlohmann::json result;
  auto* device = deviceManager.getCurrentAudioDevice();
  if (device != nullptr) {
    auto setup = deviceManager.getAudioDeviceSetup();
    result["deviceType"] = device->getTypeName().toStdString();
    result["outputDevice"] = setup.outputDeviceName.toStdString();
    result["inputDevice"] = setup.inputDeviceName.toStdString();
    result["sampleRate"] = device->getCurrentSampleRate();
    result["bufferSize"] = device->getCurrentBufferSizeSamples();

    nlohmann::json availableSampleRates = nlohmann::json::array();
    for (auto rate : device->getAvailableSampleRates()) {
      availableSampleRates.push_back(rate);
    }
    result["availableSampleRates"] = availableSampleRates;

    nlohmann::json availableBufferSizes = nlohmann::json::array();
    for (auto size : device->getAvailableBufferSizes()) {
      availableBufferSizes.push_back(size);
    }
    result["availableBufferSizes"] = availableBufferSizes;
  }
  return result;
}

juce::String AudioEngineCore::setAudioDevice(
    const juce::String& deviceTypeName, const juce::String& outputDeviceName,
    const juce::String& inputDeviceName, double sampleRate, int bufferSize) {
  // Set the device type first so the setup applies to the correct driver
  if (deviceTypeName.isNotEmpty()) {
    deviceManager.setCurrentAudioDeviceType(deviceTypeName, true);
  }

  auto setup = deviceManager.getAudioDeviceSetup();
  setup.outputDeviceName = outputDeviceName;
  setup.inputDeviceName = inputDeviceName;

  // Set sample rate if specified, otherwise keep current
  if (sampleRate > 0) {
    setup.sampleRate = sampleRate;
  }

  // Set buffer size if specified, otherwise keep current
  if (bufferSize > 0) {
    setup.bufferSize = bufferSize;
  }

  // Enable all input channels — JUCE will cap at the device's actual maximum
  setup.inputChannels.setRange(0, 64, true);
  setup.useDefaultInputChannels = false;

  auto error = deviceManager.setAudioDeviceSetup(setup, true);

  if (error.isEmpty()) {
    saveSettings();
  }

  return error;
}

nlohmann::json AudioEngineCore::getAvailableInputs() const {
  nlohmann::json inputs = nlohmann::json::array();

  auto* device = deviceManager.getCurrentAudioDevice();
  if (device != nullptr) {
    auto channelNames = device->getInputChannelNames();
    auto activeChannels = device->getActiveInputChannels();

    for (int i = 0; i < channelNames.size(); ++i) {
      if (activeChannels[i]) {
        nlohmann::json input;
        input["index"] = i;
        input["name"] = channelNames[i].toStdString();
        inputs.push_back(input);
      }
    }
  }

  return inputs;
}

int AudioEngineCore::getNumInputChannels() const {
  auto* device = deviceManager.getCurrentAudioDevice();
  if (device != nullptr) {
    return device->getActiveInputChannels().countNumberOfSetBits();
  }
  return 0;
}

void AudioEngineCore::incrementMonitoringCount() {
  monitoringTrackCount.fetch_add(1, std::memory_order_relaxed);
}

void AudioEngineCore::decrementMonitoringCount() {
  monitoringTrackCount.fetch_sub(1, std::memory_order_relaxed);
}

juce::File AudioEngineCore::getSettingsFile() const {
  return juce::File::getSpecialLocation(
             juce::File::userApplicationDataDirectory)
      .getChildFile("DAWAudioEngine")
      .getChildFile("audio-settings.xml");
}

void AudioEngineCore::saveSettings() {
  auto xml = deviceManager.createStateXml();
  if (xml == nullptr) return;

  auto file = getSettingsFile();
  file.getParentDirectory().createDirectory();
  xml->writeTo(file);

  juce::Logger::writeToLog("[AudioEngine] Settings saved to " +
                           file.getFullPathName());
}

std::unique_ptr<juce::XmlElement> AudioEngineCore::loadSettings() {
  auto file = getSettingsFile();
  if (!file.existsAsFile()) return nullptr;

  auto xml = juce::XmlDocument::parse(file);
  if (xml != nullptr) {
    juce::Logger::writeToLog("[AudioEngine] Settings loaded from " +
                             file.getFullPathName());
  }
  return xml;
}
