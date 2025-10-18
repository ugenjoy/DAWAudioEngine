#include "audio-track.hpp"
#include "wave-table.hpp"

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
  float interval;
  float frequency;
  float duration;

  // ADSR Enveloppe
  float att, dec, sus, rel;

  static WaveTable waveTable;
};