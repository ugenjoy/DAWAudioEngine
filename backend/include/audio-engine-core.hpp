#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <memory>
#include <vector>
#include "audio-track.hpp"
#include "beat-track.hpp"
#include "song.hpp"

// TODO: [MEDIUM] Add mixer functionality:
// - struct MixerBus { float volume, pan; std::vector<Effect*> effects; };
// - void setMasterVolume(float volume);
// - void setTrackPan(size_t trackIndex, float pan);

// TODO: [MEDIUM] Add error callback system:
// - std::function<void(const String& error)> errorCallback;
// - void setErrorCallback(std::function<void(const String&)> callback);

class AudioEngineCore : public juce::AudioAppComponent {
 public:
  AudioEngineCore();
  ~AudioEngineCore() override;

  // AudioAppComponent overrides
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void getNextAudioBlock(
      const juce::AudioSourceChannelInfo& bufferToFill) override;
  void releaseResources() override;

  // Song management
  void loadSong(Song* newSong) { activeSong = newSong; }

  // Play control
  void play();
  void pause();
  void stop();
  void switchPlaying();

 private:
  std::atomic<bool> playing;
  float masterVolume;

  // Pre-allocated buffers for audio processing (avoid allocations in audio
  // thread)
  juce::AudioBuffer<float> mixBuffer;  // Stereo mix buffer
  juce::AudioBuffer<float>
      trackBuffer;  // Mono buffer for individual track rendering

  Song* activeSong;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngineCore)
};