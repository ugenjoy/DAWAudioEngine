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
- **AppContext**: Service locator providing access to all core services
- **CommandFactory**: Auto-registration system for commands (zero boilerplate!)
- **WebSocketServer**: Crow-based JSON API on port 8080
- **AudioTrack**: Abstract base class for all audio track types
- **BeatTrack**: Concrete implementation generating beat-synchronized tones
- **WaveTable**: Optimized wavetable oscillator with multiple waveform types

See [backend/ARCHITECTURE.md](./backend/ARCHITECTURE.md) for detailed documentation.

### Project Structure

```
backend/
├── CMakeLists.txt          # Build configuration
├── ARCHITECTURE.md         # Detailed architecture docs
├── include/
│   ├── app-context.hpp
│   ├── audio/              # Audio engine components
│   ├── commands/           # Command pattern + factory
│   ├── model/              # Domain models (Song, Track)
│   ├── services/           # ProjectManager, SongsManager
│   └── websocket/          # WebSocket server
├── src/                    # Implementations
├── assets/
│   └── demo.dawproj/       # Demo project
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
   ./build/DAWAudioEngine_artefacts/Release/DAWAudioEngine
   ```

## 🎛️ Usage

### WebSocket API

The engine starts a WebSocket server on `ws://localhost:8080/ws`.

**Load demo project:**
```json
{"action": "project.load", "path": "/path/to/backend/assets/demo.dawproj"}
```

**Transport controls:**
```json
{"action": "transport.play"}
{"action": "transport.pause"}
{"action": "transport.stop"}
```

**Save project:**
```json
{"action": "project.save", "path": "/path/to/myproject.dawproj"}
```

**Health check:**
```bash
curl http://localhost:8080/health
```

### Adding a New Command

Commands auto-register themselves - no boilerplate!

**1. Create class** (`include/commands/my-command.hpp`):
```cpp
class MyCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
  std::string getName() const override { return "my.command"; }
};
```

**2. Implement + register** (`src/commands/my-command.cpp`):
```cpp
void MyCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().doSomething();
}

REGISTER_COMMAND("my.command", MyCommand);  // Auto-registration!
```

**3. Add to CMakeLists.txt** - Done! 🎉

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

- [x] WebSocket API with command system
- [x] Auto-registration for commands (zero boilerplate)
- [x] Project serialization (.dawproj format)
- [x] Command pattern with lock-free queue
- [ ] Audio file loading (WAV, MP3, FLAC)
- [ ] Effects processing (reverb, delay, EQ)
- [ ] MIDI support
- [ ] Multi-track recording
- [ ] Plugin system (VST3, AU)

## 📝 License

This project uses the JUCE framework. Please refer to JUCE's licensing terms for commercial use.

**Built with ❤️ using JUCE Framework**
