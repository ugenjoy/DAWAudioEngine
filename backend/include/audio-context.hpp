class AudioContext {
 public:
  static AudioContext& getInstance() {
    static AudioContext instance;
    return instance;
  }

  double sampleRate = 44100.0;
  int bufferSize = 512;

  std::atomic<float> tempoBPM{120.0f};
  std::atomic<int> timeSignatureNumerator{4};
  std::atomic<int> timeSignatureDenominator{4};

  AudioContext(const AudioContext&) = delete;

 private:
  AudioContext() = default;
};