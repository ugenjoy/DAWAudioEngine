# DAW Audio Engine

A real-time audio workstation built with a C++/JUCE backend and a Tauri/React frontend, communicating over WebSocket.

## Features

- **Real-time audio engine** — low-latency playback with ALSA/JACK support
- **Multi-track architecture** — audio file tracks, metronome, wavetable synthesis
- **WebSocket API** — JSON command protocol on `ws://localhost:8080/ws`
- **Command auto-registration** — add commands with zero boilerplate
- **Live / Edit modes** — separate performance and editing workflows
- **Programmable events** — trigger MIDI messages on transport events (e.g. send Program Change on song load)
- **Project serialization** — `.dawproj` folder format with JSON metadata

See [ARCHITECTURE.md](./ARCHITECTURE.md) for detailed technical documentation.

## Project Structure

```
daw/
├── backend/          C++ audio engine (JUCE + Crow WebSocket)
├── frontend/         Desktop app (Tauri + React + TypeScript)
├── ARCHITECTURE.md   Detailed architecture docs
└── CLAUDE.md         AI assistant instructions
```

## Prerequisites

- **CMake** >= 3.22
- **C++17** compiler (GCC 7+, Clang 5+)
- **Node.js** >= 18, **pnpm**
- **Rust** (for Tauri)
- **JUCE system dependencies** (Linux):
  ```bash
  # Debian / Ubuntu
  sudo apt-get install libasound2-dev libjack-jackd2-dev \
      ladspa-sdk libcurl4-openssl-dev libfreetype6-dev \
      libx11-dev libxcomposite-dev libxcursor-dev \
      libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
      libwebkit2gtk-4.1-dev libglu1-mesa-dev mesa-common-dev

  # Arch Linux
  sudo pacman -S alsa-lib jack2 freetype2 libx11 libxcomposite \
      libxcursor libxext libxinerama libxrandr libxrender \
      webkit2gtk-4.1 glu mesa
  ```

## Quick Start

### 1. Clone and init submodules

```bash
git clone <repository-url>
cd daw
git submodule update --init --recursive
```

### 2. Build and run the backend

```bash
cd backend
make build   # cmake + make
make run     # starts the audio engine on port 8080
```

Health check: `curl http://localhost:8080/health`

### 3. Build and run the frontend

```bash
cd frontend
pnpm install
pnpm dev     # Tauri dev mode with hot reload
```

The frontend connects to `ws://localhost:8080/ws` automatically.

## Backend Commands

| Command | Description |
|---|---|
| `make build` | Build the audio engine |
| `make run` | Run the audio engine |
| `make tests` | Build and run unit tests |
| `make clean` | Clean build artifacts |

Manual build:
```bash
cd backend/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./DAWAudioEngine_artefacts/Release/DAWAudioEngine
```

## Frontend Commands

| Command | Description |
|---|---|
| `pnpm dev` | Dev mode with hot reload |
| `pnpm build` | Production build |
| `pnpm build:linux` | Linux package |
| `pnpm lint` | ESLint check |
| `pnpm format` | Prettier format |

Type checking:
```bash
npx tsc --noEmit
```

## WebSocket API

Connect to `ws://localhost:8080/ws` and send JSON messages.

### Transport

```json
{"action": "transport.play"}
{"action": "transport.pause"}
{"action": "transport.stop"}
{"action": "transport.setCursorPosition", "position": 0.0}
{"action": "transport.setMasterVolume", "volume": 0.8}
```

### Project

```json
{"action": "project.load", "path": "/path/to/project.dawproj"}
{"action": "project.save", "path": "/path/to/project.dawproj"}
{"action": "project.list"}
{"action": "project.getLoaded"}
{"action": "project.loadSong", "uuid": "..."}
```

### Tracks

```json
{"action": "audio.listDevices"}
{"action": "audio.setDevice", "deviceType": "ALSA", "outputDevice": "...", "inputDevice": "...", "sampleRate": 48000, "bufferSize": 256}
{"action": "audio.listInputs"}
{"action": "track.setInput", "trackId": "...", "inputChannel": 0, "stereo": false}
{"action": "track.setMonitoring", "trackId": "...", "monitoring": true}
{"action": "track.setMute", "trackId": "...", "mute": true}
{"action": "track.setSolo", "trackId": "...", "solo": true}
{"action": "track.setVolume", "trackId": "...", "volume": 0.5}
```

### Song

```json
{"action": "song.setTempo", "tempo": 140}
{"action": "song.setMetronomeMute", "mute": false}
```

### Events

```json
{"action": "event.list", "scope": "project"}
{"action": "event.add", "scope": "song", "trigger": "song.loaded", "eventAction": {"type": "midi.send", "params": {"device": "...", "message": [192, 0]}}}
{"action": "event.remove", "scope": "project", "eventId": "..."}
{"action": "event.update", "scope": "song", "eventId": "...", "trigger": "song.loaded", "enabled": true, "eventAction": {"type": "midi.send", "params": {"device": "...", "message": [192, 5]}}}
```

### MIDI

```json
{"action": "midi.listOutputs"}
{"action": "midi.send", "device": "...", "message": [192, 0]}
```

### Response Format

```json
{"type": "ack", "action": "transport.play", "status": "queued"}
{"type": "error", "message": "Unknown action"}
{"type": "broadcast", "event": "transport.play"}
{"type": "response", "event": "project.currentLoaded", ...}
```

## Adding a New Command

Commands auto-register themselves — no changes needed in `main.cpp`.

**1. Header** (`include/commands/my-command.hpp`):
```cpp
#pragma once
#include "commands/command.hpp"

class MyCommand : public Command {
 public:
  explicit MyCommand(std::string param);
  void execute(AppContext& ctx) override;
 private:
  std::string param;
};
```

**2. Implementation** (`src/commands/my-command.cpp`):
```cpp
#include "commands/my-command.hpp"
#include "commands/command-factory.hpp"
#include "app-context.hpp"

MyCommand::MyCommand(std::string param) : param(std::move(param)) {}

void MyCommand::execute(AppContext& ctx) {
  ctx.getAudioEngine().doSomething();
}

// One-line auto-registration
REGISTER_COMMAND_WITH_CREATOR(
    "my.command", MyCommand,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string param = payload.value("param", "");
      if (param.empty()) return nullptr;
      return std::make_unique<MyCommand>(param);
    });
```

**3. Add source to `backend/CMakeLists.txt`** in `target_sources()`.

Use `REGISTER_EDIT_COMMAND_WITH_CREATOR` for commands restricted to Edit mode.

## Testing

```bash
cd backend/build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest --output-on-failure
```

Test coverage: WaveTable generation.

## Raspberry Pi

See [RASPBERRY_PI_BUILD.md](./RASPBERRY_PI_BUILD.md) for cross-compilation and deployment instructions.

## License

This project uses the JUCE framework. Please refer to JUCE's licensing terms for commercial use.
