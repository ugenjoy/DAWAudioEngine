class AudioTrack {
 public:
  AudioTrack();
  virtual ~AudioTrack() = default;

  virtual float getSampleValue(double sampleTime) = 0;

 protected:
  float volume;
  float pan;
  bool mute;
};