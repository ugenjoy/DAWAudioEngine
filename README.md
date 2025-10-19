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

## 🎛️ Usage

The current implementation automatically starts playback on launch and generates a beat at 120 BPM (500 Hz sine wave).

### Customizing Audio Context

```cpp
auto& ctx = AudioContext::getInstance();
ctx.tempoBPM = 140.0f;              // Set tempo to 140 BPM
ctx.timeSignatureNumerator = 3;     // 3/4 time signature
ctx.timeSignatureDenominator = 4;
```

## 🧪 Testing

Unit tests are located in the `tests/` directory and use JUCE's built-in testing framework.

### Building Tests

```bash
cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest --output-on-failure
```

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

## 🗺️ Roadmap

- [ ] Implement minimal API with basic controls
- [ ] Implement audio file loading (WAV, MP3, FLAC)
- [ ] Add effects processing (reverb, delay, EQ)
- [ ] Implement MIDI support
- [ ] Add GUI interface
- [ ] Multi-track recording
- [ ] Plugin system (VST3, AU)

## 📝 License

This project uses the JUCE framework. Please refer to JUCE's licensing terms for commercial use.

**Built with ❤️ using JUCE Framework**
