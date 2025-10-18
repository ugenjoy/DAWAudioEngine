class AudioContext {
 public:
  static AudioContext& getInstance() {
    static AudioContext instance;
    return instance;
  }

  double sampleRate = 44100.0;
  int bufferSize = 512;

  float tempoBPM = 120.0f;
  int timeSignatureNumerator = 4;
  int timeSignatureDenominator = 4;

  AudioContext(const AudioContext&) = delete;

 private:
  AudioContext() = default;
};