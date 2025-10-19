# DAW Audio Engine

A real-time audio engine built with C++ and JUCE framework for digital audio workstation applications.

## 🎵 Features

- **Real-time audio processing** with low-latency playback
- **Wavetable synthesis** with optimized lookup tables
- **ADSR envelope generator** for dynamic sound shaping
- **Beat-synchronized tracks** with configurable tempo
- **Multi-track architecture** supporting multiple audio sources
- **ALSA and JACK support** for Linux audio backends

## 🏗️ Architecture

### Core Components

- **AudioEngineCore**: Main audio engine managing playback and track mixing
- **AudioContext**: Singleton providing global audio configuration (sample rate, tempo, time signature)
- **AudioTrack**: Abstract base class for all audio track types
- **BeatTrack**: Concrete implementation generating beat-synchronized tones
- **WaveTable**: Optimized wavetable oscillator with multiple waveform types

### Project Structure

```
backend/
├── CMakeLists.txt          # Build configuration
├── Makefile                # Convenience build wrapper
├── include/                # Header files
│   ├── audio-context.hpp
│   ├── audio-engine-core.hpp
│   ├── audio-track.hpp
│   ├── beat-track.hpp
│   └── wave-table.hpp
├── src/                    # Implementation files
│   ├── audio-engine-core.cpp
│   ├── audio-track.cpp
│   ├── beat-track.cpp
│   └── main.cpp
├── tests/                  # Unit tests
│   ├── test.wavetable.cpp
│   └── test.beattrack.cpp
└── JUCE/                   # JUCE framework (submodule)
```

## 🚀 Getting Started

### Prerequisites

- **CMake** >= 3.22
- **C++17** compatible compiler (GCC 7+, Clang 5+)
- **JUCE dependencies** (Linux):
  ```bash
  sudo apt-get install libasound2-dev libjack-jackd2-dev \
      ladspa-sdk libcurl4-openssl-dev libfreetype6-dev \
      libx11-dev libxcomposite-dev libxcursor-dev \
      libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
      libwebkit2gtk-4.1-dev libglu1-mesa-dev mesa-common-dev
  ```

### Building

1. **Clone the repository**:

   ```bash
   git clone <repository-url>
   cd daw/backend
   ```

2. **Initialize JUCE submodule** (if not already done):

   ```bash
   git submodule update --init --recursive
   ```

3. **Build the project**:

   ```bash
   make build
   ```

   Or manually:

   ```bash
   mkdir -p build
   cd build
   cmake ..
   make -j$(nproc)
   ```

4. **Run the audio engine**:
   ```bash
   ./build/DAWAudioEngine_artefacts/Debug/DAWAudioEngine
   ```

### Building Tests

```bash
cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest --output-on-failure
```

## 🎛️ Usage

The current implementation automatically starts playback on launch and generates a beat at 120 BPM (500 Hz sine wave).

### Basic Example

```cpp
#include "audio-engine-core.hpp"

int main() {
    // Create and initialize the audio engine
    AudioEngineCore engine;

    // Audio starts automatically
    // Press Ctrl+C to quit

    return 0;
}
```

### Customizing Audio Context

```cpp
auto& ctx = AudioContext::getInstance();
ctx.tempoBPM = 140.0f;              // Set tempo to 140 BPM
ctx.timeSignatureNumerator = 3;     // 3/4 time signature
ctx.timeSignatureDenominator = 4;
```

## 🧪 Testing

Unit tests are located in the `tests/` directory and use JUCE's built-in testing framework.

### Running Tests

```bash
cd build
./DAWAudioEngine_Tests
```

### Test Coverage

- **WaveTable Tests**: Waveform generation, phase wrapping, interpolation
- **BeatTrack Tests**: ADSR envelope, timing, volume control

## 🔧 Configuration

### Audio Settings

- **Sample Rate**: Configured automatically by the audio device (typically 44100 Hz or 48000 Hz)
- **Buffer Size**: Configured by the audio device (typically 256-512 samples)
- **Channels**: Stereo output (2 channels)

### Build Options

Edit `CMakeLists.txt` to customize:

```cmake
set(CMAKE_CXX_STANDARD 17)          # C++ standard version
JUCE_ALSA=1                          # Enable ALSA support
JUCE_JACK=1                          # Enable JACK support
```

## 📊 Performance

- **Wavetable Lookup**: O(1) with optional linear interpolation
- **Real-time Safe**: No dynamic memory allocation in audio callback
- **Thread-safe**: Atomic operations for shared parameters

## 🛠️ Development

### Code Style

- Use C++17 features
- Follow JUCE coding conventions
- Use `camelCase` for variables and methods
- Use `PascalCase` for classes
- Always use include guards (`#pragma once`)
- All comments in English

### Adding a New Track Type

1. Create header in `include/`:

   ```cpp
   #pragma once
   #include "audio-track.hpp"

   class MyTrack : public AudioTrack {
   public:
       float getSampleValue(double sampleTime) override;
   };
   ```

2. Implement in `src/`:

   ```cpp
   #include "my-track.hpp"

   float MyTrack::getSampleValue(double sampleTime) {
       // Your audio generation code
       return 0.0f;
   }
   ```

3. Add to `AudioEngineCore`:
   ```cpp
   tracks.push_back(std::make_unique<MyTrack>());
   ```

## 🐛 Known Issues

- No play/pause/stop controls (auto-starts on launch)
- Limited to one beat track type
- No audio file loading/playback yet
- No effects processing chain

## 🗺️ Roadmap

- [ ] Add play/pause/stop controls
- [ ] Implement audio file loading (WAV, MP3, FLAC)
- [ ] Add effects processing (reverb, delay, EQ)
- [ ] Implement MIDI support
- [ ] Add GUI interface
- [ ] Multi-track recording
- [ ] Plugin system (VST3, AU)

## 📝 License

This project uses the JUCE framework. Please refer to JUCE's licensing terms for commercial use.

## 🤝 Contributing

Contributions are welcome! Please ensure:

1. Code follows the project style
2. All tests pass
3. New features include tests
4. Comments are in English

## 📧 Contact

For questions or suggestions, please open an issue on the repository.

---

## 🚧 TODO & Development Roadmap

### Critical Priority (Performance)

- [ ] **[CRITICAL]** Eliminate allocations in audio callback thread
- [ ] **[CRITICAL]** Pre-allocate audio buffers for mixing
- [ ] **[HIGH]** Replace floating-point time accumulation with sample counter
- [ ] **[HIGH]** Add lock-free structures for thread-safe track management

### High Priority (Features)

- [ ] **[HIGH]** Implement dynamic track management API (add/remove tracks)
- [ ] **[HIGH]** Add error handling for audio device failures
- [ ] **[MEDIUM]** Implement audio mixer with bus routing
- [ ] **[MEDIUM]** Add panning control per track
- [ ] **[MEDIUM]** Make ADSR parameters configurable

### Medium Priority (Quality)

- [ ] **[MEDIUM]** Add tempo change ramping to avoid clicks
- [ ] **[MEDIUM]** Implement track solo/mute logic
- [ ] **[MEDIUM]** Add configuration file support (JSON/XML)
- [ ] **[MEDIUM]** Add effects chain support (reverb, delay, EQ)

### Low Priority (Optimizations)

- [ ] **[LOW]** Add SIMD optimizations for WaveTable
- [ ] **[LOW]** Implement bandlimited waveforms (PolyBLEP)
- [ ] **[LOW]** Add cubic interpolation for WaveTable
- [ ] **[LOW]** Add velocity sensitivity to ADSR
- [ ] **[LOW]** Add waveform selection API
- [ ] **[LOW]** Add preset management system
- [ ] **[LOW]** Add performance benchmarks

### Documentation

- [ ] Add architecture diagrams
- [ ] Add API usage examples
- [ ] Add performance profiling guide
- [ ] Add contribution guidelines

### Finding TODOs in Code

Search the codebase for prioritized TODO comments:

```bash
grep -r "TODO: \[CRITICAL\]" backend/
grep -r "TODO: \[HIGH\]" backend/
grep -r "TODO: \[MEDIUM\]" backend/
grep -r "TODO: \[LOW\]" backend/
```

---

**Built with ❤️ using JUCE Framework**
