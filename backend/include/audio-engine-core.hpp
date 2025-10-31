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

// TODO: [HIGH] Add dynamic track management API:
// - void addTrack(std::unique_ptr<AudioTrack> track);
// - void removeTrack(size_t index);
// - AudioTrack* getTrack(size_t index);
// - size_t getTrackCount() const;

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

 private:
  bool playing;
  double currentPosition;  // TODO: [MEDIUM] Replace with int64_t totalSampleCount
  float masterVolume;

  // Pre-allocated buffers for audio processing (avoid allocations in audio thread)
  juce::AudioBuffer<float> mixBuffer;  // Stereo mix buffer
  juce::AudioBuffer<float> trackBuffer;  // Mono buffer for individual track rendering
  std::vector<float> trackPanValues;

  // TODO: [HIGH] Add thread-safe track management:
  // juce::SpinLock trackLock;  // or use lock-free structure

  std::vector<std::unique_ptr<AudioTrack>> tracks;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngineCore)
};