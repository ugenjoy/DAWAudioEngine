# DAW Backend Architecture

## Overview

The backend is a C++ application built on JUCE framework providing real-time audio processing with a WebSocket server for frontend communication. It follows a command-based architecture enabling thread-safe communication between network and audio layers.

## Directory Structure

```
backend/
├── include/
│   ├── app-context.hpp           # Application context (service locator)
│   ├── audio/                    # Audio engine components
│   │   ├── audio-context.hpp     # Global audio parameters (singleton)
│   │   ├── audio-engine-core.hpp # Main audio engine
│   │   ├── audio-track.hpp       # Abstract track base class
│   │   ├── beat-track.hpp        # Beat-synchronized track
│   │   └── dsp/
│   │       └── wave-table.hpp    # Wavetable oscillator
│   ├── commands/                 # Command pattern implementation
│   │   ├── command.hpp           # Command interface
│   │   ├── command-factory.hpp   # Command factory
│   │   ├── command-queue.hpp     # Lock-free SPSC queue
│   │   ├── command-processor.hpp # Command worker thread
│   │   ├── transport-commands.hpp
│   │   └── project-commands.hpp
│   ├── model/
│   │   ├── song.hpp
│   │   └── tracks-manager.hpp
│   ├── services/
│   │   ├── project-manager.hpp
│   │   └── songs-manager.hpp
│   └── websocket/
│       └── websocket-server.hpp
├── src/                          # Implementation files
├── assets/
│   └── demo.dawproj/             # Demo project
└── JUCE/                         # JUCE framework (submodule)
```

## Core Components

### AppContext

Central service locator providing access to all core services. Passed to commands for cross-subsystem operations.

```cpp
class AppContext {
  AudioEngineCore& audioEngine;
  SongsManager& songsManager;
  ProjectManager& projectManager;
};
```

### AudioEngineCore

Main JUCE `AudioAppComponent` managing:
- Audio device initialization (stereo output)
- Real-time audio rendering callback
- Playback control (play/pause/stop)
- Active song management

### Command System

Decouples WebSocket thread from audio operations via a producer-consumer pattern with a factory:

```
WebSocket Thread          Worker Thread           Audio Thread
      │                        │                       │
      │ CommandFactory.create()│                       │
      │         │              │                       │
      │   push(Command)        │                       │
      ├───────────────────────►│                       │
      │                        │   execute(ctx)        │
      │                        ├──────────────────────►│
      │                        │                       │
```

**Components:**
- `Command` - Abstract interface for all commands
- `CommandFactory` - Creates commands from action strings (registered in main.cpp)
- `CommandQueue` - Lock-free SPSC queue (JUCE AbstractFifo)
- `CommandProcessor` - Worker thread polling the queue

### CommandFactory

Factory pattern with **auto-registration** system. Commands register themselves using macros at static initialization time:

```cpp
// In transport-commands.cpp
void PlayCommand::execute(AppContext& ctx) { ctx.getAudioEngine().play(); }

// Auto-registration (one line per command)
REGISTER_COMMAND("transport.play", PlayCommand);
```

For commands with parameters:
```cpp
// In project-commands.cpp
static CommandRegistrar registerLoadProject(
    "project.load", [](const nlohmann::json& payload) -> CommandPtr {
      std::string path = payload.value("path", "");
      if (path.empty()) return nullptr;
      return std::make_unique<LoadProjectCommand>(path);
    });
```

**How it works:**
1. Macros create static objects that register commands before `main()` runs
2. Global registry (Meyer's singleton) collects all registrations
3. `CommandFactory::create()` copies registry into factory instance

**Benefits:**
- Zero boilerplate in `main.cpp`
- Commands self-contained (declaration + registration in same file)
- Easy to add new commands (create class + add one line)
- WebSocketServer completely decoupled from command types

### WebSocketServer

Crow-based WebSocket server:
- Runs in dedicated background thread
- Parses JSON messages
- Creates and queues commands
- Sends acknowledgments/errors

---

## WebSocket Protocol

### Connection

Connect to `ws://localhost:8080/ws`

On connection, server sends:
```json
{
  "type": "connected",
  "message": "Connected to DAW Audio Engine"
}
```

### Request Format

```json
{
  "action": "<action_name>",
  "param1": "value1",
  "param2": "value2"
}
```

### Response Format

**Success:**
```json
{
  "type": "ack",
  "action": "<action_name>",
  "status": "queued"
}
```

**Error:**
```json
{
  "type": "error",
  "message": "<error_description>"
}
```

**Broadcast (sent to all connected clients):**
```json
{
  "type": "broadcast",
  "event": "<event_name>",
  ... event-specific data ...
}
```

### Broadcast Events

Commands can broadcast events to all connected clients. This allows real-time notifications of state changes.

**Example - Project Loaded:**
```json
{
  "type": "broadcast",
  "event": "project.loaded",
  "path": "/path/to/project.dawproj",
  "songsCount": 2
}
```

**Example - Project Load Failed:**
```json
{
  "type": "broadcast",
  "event": "project.loadFailed",
  "path": "/path/to/project.dawproj",
  "error": "File not found"
}
```

### Available Actions

#### Transport

| Action | Parameters | Description |
|--------|------------|-------------|
| `transport.play` | - | Start playback |
| `transport.pause` | - | Pause playback |
| `transport.stop` | - | Stop and reset position |

#### Project

| Action | Parameters | Description |
|--------|------------|-------------|
| `project.load` | `path`: string | Load project from disk |
| `project.save` | `path`: string | Save project to disk |

### Examples

**Start playback:**
```json
{"action": "transport.play"}
```

**Load project:**
```json
{"action": "project.load", "path": "/home/user/myproject.dawproj"}
```

---

## Project Serialization

### File Structure

Projects are stored as folders with `.dawproj` extension:

```
MyProject.dawproj/
├── project.json    # Main data file
├── audio/          # Audio samples (future)
└── cache/          # Cached data (future)
```

### JSON Schema

**project.json:**
```json
{
  "version": "1.0.0",
  "songs": [
    {
      "id": "uuid-string",
      "tempo": 120,
      "tracks": [...]
    }
  ]
}
```

**Song:**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "tempo": 120,
  "tracks": [
    { /* track object */ }
  ]
}
```

**BeatTrack:**
```json
{
  "type": "BeatTrack",
  "id": "track-uuid",
  "volume": 0.5,
  "pan": 0.0,
  "mute": false,
  "frequency": 440.0
}
```

### Track Types

| Type | Description | Specific Fields |
|------|-------------|-----------------|
| `BeatTrack` | Beat-synchronized tone generator | `frequency` (Hz) |

---

## Thread Model

```
┌─────────────────────────────────────────────────────────────┐
│                      Main Thread (JUCE)                     │
│                   Application lifecycle                      │
└─────────────────────────────────────────────────────────────┘
         │
         ├──────────────────┬──────────────────┐
         ▼                  ▼                  ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│  Audio Thread   │ │ WebSocket Thread│ │ Command Thread  │
│  (Real-time)    │ │   (Blocking)    │ │   (Worker)      │
│                 │ │                 │ │                 │
│ getNextAudio    │ │ Crow server     │ │ Poll queue      │
│ Block()         │ │ JSON parsing    │ │ Execute cmds    │
└─────────────────┘ └─────────────────┘ └─────────────────┘
```

### Thread Safety

| Component | Mechanism | Protected Resource |
|-----------|-----------|-------------------|
| `AudioEngineCore` | `std::atomic<bool>` | Playing state |
| `AudioContext` | `std::atomic<int>` | Time signature |
| `CommandQueue` | JUCE `AbstractFifo` | Command buffer |
| `WebSocketServer` | `std::atomic<bool>` | Running state |

---

## Audio Rendering Pipeline

```
AudioEngineCore::getNextAudioBlock()
    │
    ├── Clear mix buffer
    │
    ├── Song::render()
    │   │
    │   └── TracksManager::renderTracks()
    │       │
    │       └── For each track:
    │           └── BeatTrack::renderBlock()
    │               │
    │               ├── Calculate phase
    │               ├── WaveTable lookup
    │               └── Apply ADSR envelope
    │
    └── Apply master volume
```

### Real-Time Constraints

- Pre-allocated buffers (no allocations in audio callback)
- Lock-free command queue
- Atomic state variables only

---

## Adding New Features

### Adding a New Command

1. Create header in `include/commands/`:
```cpp
// my-command.hpp
#pragma once
#include "commands/command.hpp"

class MyCommand : public Command {
 public:
  explicit MyCommand(std::string param);
  void execute(AppContext& ctx) override;
  std::string getName() const override { return "my.command"; }
 private:
  std::string param;
};
```

2. Create implementation in `src/commands/`:
```cpp
// my-command.cpp
#include "commands/my-command.hpp"
#include "commands/command-factory.hpp"
#include "app-context.hpp"

MyCommand::MyCommand(std::string param) : param(std::move(param)) {}

void MyCommand::execute(AppContext& ctx) {
  // Implementation using ctx.getAudioEngine(), ctx.getSongsManager(), etc.
}

// Auto-registration
static CommandRegistrar registerMyCommand(
    "my.command", [](const nlohmann::json& payload) -> CommandPtr {
      std::string param = payload.value("param", "");
      if (param.empty()) return nullptr;
      return std::make_unique<MyCommand>(param);
    });
```

3. Add source file to `CMakeLists.txt`

**That's it!** No changes needed in `main.cpp` or `WebSocketServer`. The command is automatically registered at startup.

### Adding a New Track Type

1. Create class inheriting from `AudioTrack`
2. Implement `getSampleValue()`, `renderBlock()`, `toJson()`, `getTrackType()`
3. Add `fromJson()` static factory method
4. Register in `TracksManager::loadFromJson()`

---

## Dependencies

| Library | Purpose | Version |
|---------|---------|---------|
| JUCE | Audio framework | 7.x |
| Crow | WebSocket server | 1.2.0 |
| ASIO | Async I/O (Crow dep) | 1.30.2 |
| nlohmann/json | JSON serialization | 3.11.3 |
| ALSA | Audio driver (Linux) | System |
| JACK | Audio driver (optional) | System |

---

## Health Check

HTTP endpoint available at `http://localhost:8080/health` returns `200 OK`.
