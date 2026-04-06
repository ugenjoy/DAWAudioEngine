#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

#include "audio/audio-track.hpp"
#include "audio/metronome-track.hpp"
#include "model/song.hpp"
#include "services/loop-manager.hpp"
#include "websocket/websocket-server.hpp"

// TODO: [MEDIUM] Add error callback system:
// - std::function<void(const String& error)> errorCallback;
// - void setErrorCallback(std::function<void(const String&)> callback);

class AudioEngineCore : public juce::AudioIODeviceCallback,
                        public juce::Timer {
 public:
  AudioEngineCore();
  ~AudioEngineCore() override;

  // AudioIODeviceCallback overrides
  void audioDeviceIOCallbackWithContext(
      const float* const* inputChannelData, int numInputChannels,
      float* const* outputChannelData, int numOutputChannels, int numSamples,
      const juce::AudioIODeviceCallbackContext& context) override;
  void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
  void audioDeviceStopped() override;

  // Song management
  void loadSong(Song* newSong);
  void unloadSong();
  Song* getActiveSong() { return activeSong; }

  // Transport control
  void play();
  void pause();
  void stop();
  void switchPlaying();
  bool isPlaying() const { return playing.load(); }
  void setPlayheadPosition(double position);
  void setCursorPosition(double position);

  double getPlayheadPosition() { return playheadPosition; };
  double getCursorPosition() { return cursorPosition; };

  // Master volume
  void setMasterVolume(float volume);
  float getMasterVolume() const { return masterVolume.load(); }

  // Mode-driven optimizations
  void setMonitoringEnabled(bool enabled);

  /**
   * Rebuild the bitmask of input channels needed by monitoring tracks.
   * Call after any monitoring or input channel change.
   * Thread-safe: result is stored in an atomic.
   */
  void rebuildMonitoredChannelMask();

  /** Pre-render all freezable tracks into static buffers (Live mode). */
  void freezeTracks();

  /** Release all frozen buffers (Edit mode). */
  void unfreezeTracks();

  // Timer override (broadcasts transport position to clients)
  void timerCallback() override;

  // Audio devices
  nlohmann::json getAvailableDevices();
  nlohmann::json getCurrentDeviceInfo() const;
  juce::String setAudioDevice(const juce::String& deviceTypeName,
                              const juce::String& outputDeviceName,
                              const juce::String& inputDeviceName,
                              double sampleRate = 0,
                              int bufferSize = 0);

  // Audio inputs
  nlohmann::json getAvailableInputs() const;
  int getNumInputChannels() const;
  void incrementMonitoringCount();
  void decrementMonitoringCount();

  // WS
  void setWebSocketServer(WebSocketServer* server) { wsServer = server; }

  // End position callback
  using EndPositionCallback = std::function<void()>;
  void setEndPositionCallback(EndPositionCallback cb);
  void setLoopManager(LoopManager* lm);

 private:
  juce::AudioDeviceManager deviceManager;

  std::atomic<bool> playing;
  std::atomic<float> masterVolume{1.0f};
  std::atomic<bool> monitoringEnabled{true};
  std::atomic<uint64_t> monitoredChannelMask{0};

  // Pre-allocated buffers for audio processing (avoid allocations in audio
  // thread)
  juce::AudioBuffer<float> mixBuffer;    // Stereo mix buffer
  juce::AudioBuffer<float> trackBuffer;  // Stereo buffer for individual track rendering
  juce::AudioBuffer<float> inputBuffer;  // Copy of audio input channels

  std::atomic<int> monitoringTrackCount{0};

  Song* activeSong;
  std::atomic<double> playheadPosition;
  std::atomic<double> cursorPosition;

  WebSocketServer* wsServer = nullptr;

  EndPositionCallback endPositionCallback;
  std::atomic<bool> endPositionFired{false};
  LoopManager* loopManager = nullptr;

  // Audio settings persistence
  void saveSettings();
  std::unique_ptr<juce::XmlElement> loadSettings();
  juce::File getSettingsFile() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngineCore)
};
