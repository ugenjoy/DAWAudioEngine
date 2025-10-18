#include "audio-track.hpp"

class BeatTrack : public AudioTrack {
 public:
  BeatTrack(float freqency);
  ~BeatTrack() override;

  float getSampleValue(double sampleTime) override;

  void setMute(bool mute);
  void setVolume(float volume);

 private:
  float computeEnveloppe(float timeSinceLastBeat);

  // Beat
  float phase;
  float interval;
  float frequency;
  float duration;

  // Enveloppe
  float att;  // in s
  float dec;  // in s
  float sus;  // volume between 0 and 1
  float rel;  // in s
};