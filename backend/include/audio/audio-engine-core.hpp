#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <memory>
#include <vector>

#include <nlohmann/json.hpp>

#include "audio/audio-track.hpp"
#include "audio/beat-track.hpp"
#include "model/song.hpp"
#include "websocket/websocket-server.hpp"

// TODO: [MEDIUM] Add mixer functionality:
// - struct MixerBus { float volume, pan; std::vector<Effect*> effects; };
// - void setMasterVolume(float volume);
// - void setTrackPan(size_t trackIndex, float pan);

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
  Song* getActiveSong() { return activeSong; }

  // Transport control
  void play();
  void pause();
  void stop();
  void switchPlaying();
  void setPlayheadPosition(double position);
  void setCursorPosition(double position);

  double getPlayheadPosition() { return playheadPosition; };
  double getCursorPosition() { return cursorPosition; };

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

 private:
  juce::AudioDeviceManager deviceManager;

  std::atomic<bool> playing;
  float masterVolume;

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

  // Audio settings persistence
  void saveSettings();
  std::unique_ptr<juce::XmlElement> loadSettings();
  juce::File getSettingsFile() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngineCore)
};
