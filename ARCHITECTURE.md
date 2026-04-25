# Architecture

This document describes the internal architecture of the DAW Audio Engine project.

## Overview

The system is split into two processes communicating over WebSocket:

```
┌─────────────────────────┐       WebSocket (JSON)       ┌───────────────────────┐
│   Backend (C++ / JUCE)  │ ◄──────────────────────────► │ Frontend (Tauri/React)│
│   Port 8080             │                              │                       │
└─────────────────────────┘                              └───────────────────────┘
```

The backend owns all audio processing and state. The frontend is a thin UI that sends commands and reacts to broadcasts.

## Thread Model

```
Main Thread (JUCE message loop)
│
├── Audio Thread (real-time, high priority)
│   └── audioDeviceIOCallbackWithContext()
│       └── Song::render() → mix all tracks into output buffer
│
├── WebSocket Thread (Crow event loop)
│   └── onMessage() → parse JSON → CommandFactory::create() → CommandQueue::push()
│
├── Command Worker Thread
│   └── CommandProcessor::run() → polls CommandQueue::pop() → Command::execute()
│
└── Timer Thread (JUCE timer)
    └── timerCallback() → broadcasts playhead position to clients
```

### Thread safety rules

- **Audio thread**: no allocations, no locks, no blocking. Only atomic reads/writes.
- **WebSocket thread**: receives JSON, creates command objects, pushes to lock-free queue.
- **Worker thread**: executes commands, can use any API freely (JUCE, file I/O, broadcasts).
- **Communication**: `CommandQueue` is a lock-free SPSC queue (JUCE `AbstractFifo`, capacity 1024). All shared state between audio and worker threads uses `std::atomic` with `memory_order_relaxed`.

## Command System

Commands are the only way to mutate application state from WebSocket clients.

### Flow

```
Client JSON → WebSocketServer → CommandFactory → CommandQueue → CommandProcessor → Command::execute()
                                                  (lock-free)    (worker thread)
```

### Auto-registration

Commands register themselves at static initialization time using macros. No boilerplate in `main.cpp` or the WebSocket server.

Four registration macros:

| Macro                                | Mode        | Constructor               |
| ------------------------------------ | ----------- | ------------------------- |
| `REGISTER_COMMAND`                   | Live + Edit | Default (no payload)      |
| `REGISTER_EDIT_COMMAND`              | Edit only   | Default (no payload)      |
| `REGISTER_COMMAND_WITH_CREATOR`      | Live + Edit | Custom lambda parses JSON |
| `REGISTER_EDIT_COMMAND_WITH_CREATOR` | Edit only   | Custom lambda parses JSON |

The macros create static `CommandRegistrar` objects that populate a global registry before `main()` runs. `CommandFactory::create()` copies this registry into the factory instance.

### Access control

Commands registered with `REGISTER_EDIT_*` are rejected when the application is in Live mode. The `ModeManager` enforces this at the WebSocket layer.

## AppContext (Service Locator)

`AppContext` provides commands with access to all core services:

```cpp
ctx.getAudioEngine()       // AudioEngineCore — playback, devices, transport
ctx.getSongsManager()      // SongsManager — song collection, project events
ctx.getProjectManager()    // ProjectManager — save/load .dawproj files
ctx.getWebSocketServer()   // WebSocketServer — broadcast to clients
ctx.getModeManager()       // ModeManager — Live/Edit mode
ctx.getEventEngine()       // EventEngine — trigger-based event dispatch
ctx.getMidiOutputManager() // MidiOutputManager — MIDI device I/O
```

Services are injected as references at startup. AppContext does not own them.

## Audio Engine

### AudioEngineCore

Implements `juce::AudioIODeviceCallback`. Core responsibilities:

- **Transport**: play / pause / stop, playhead and cursor position tracking
- **Rendering**: delegates to `Song::render()` which mixes all tracks
- **Input monitoring**: copies hardware inputs to tracks based on a bitmask
- **Device management**: ALSA/JACK selection, sample rate, buffer size
- **Settings persistence**: saves/loads audio device config to XML

### Rendering pipeline

```
audioDeviceIOCallbackWithContext()
  ├── Copy monitored input channels (bitmask-driven)
  ├── Song::render(mixBuffer, trackBuffer, inputBuffer)
  │   └── TracksManager::renderTracks()
  │       ├── MetronomeTrack::renderBlock()  (wavetable + ADSR)
  │       └── AudioFileTrack::renderBlock()  (clip playback)
  ├── Apply master volume
  └── Copy mixBuffer → output
```

All buffers are pre-allocated in `audioDeviceAboutToStart()`. Zero allocations in the callback.

### Transport: stop() behavior

Stop has two phases:

1. **While playing**: stops playback, returns playhead to cursor position.
2. **While paused**: resets both cursor and playhead to zero.

### Track freezing

In Live mode, tracks that support it (MetronomeTrack) can be "frozen" — pre-rendered into a buffer on the worker thread. The audio thread then reads from the frozen buffer instead of computing samples in real-time. This reduces CPU load during performance.

### Input monitoring

Each track can monitor a hardware input channel. The engine maintains a 64-bit bitmask (`monitoredChannelMask`) where each bit represents a hardware input channel. Only channels with at least one monitoring track are copied from the input buffer, avoiding unnecessary work.

## Track Types

| Type             | Description                                | Freezable               |
| ---------------- | ------------------------------------------ | ----------------------- |
| `AudioTrack`     | Abstract base class                        | —                       |
| `MetronomeTrack` | Beat-synchronized click (wavetable + ADSR) | Yes                     |
| `AudioFileTrack` | Plays audio clips from files               | No (position-dependent) |

All tracks implement `renderBlock()` for batch processing and `toJson()` / `fromJson()` for serialization.

## Song Model

A `Song` contains:

- **TracksManager** — ordered collection of audio tracks
- **MetronomeTrack** — dedicated beat click (not in TracksManager)
- **Event rules** — song-level trigger→action rules
- **Tempo** (BPM), **name**, **UUID**

`Song::render()` is called on the audio thread and delegates to TracksManager to mix all tracks.

## Project Serialization

Projects are folders with `.dawproj` extension:

```
MyProject.dawproj/
├── project.json        Main data file
├── audio/              Audio samples (future)
└── cache/              Cached waveforms (future)
```

`project.json` schema:

```json
{
  "version": "1.0.0",
  "songs": [
    {
      "id": "uuid",
      "name": "Song 1",
      "tempo": 120,
      "tracks": [...],
      "events": [...]
    }
  ],
  "projectEvents": [...]
}
```

`ProjectManager` handles reading/writing. `SongsManager` owns the in-memory song collection.

## Event System

A programmable trigger→action system that fires MIDI messages (or other actions) in response to transport events.

### Architecture

```
EventEngine
├── rules: Vec<EventRule>          Combined project + song rules
├── executors: Map<String, ActionExecutor>
│   └── "midi.send" → MidiActionExecutor
│
│  fire("song.loaded", ctx)
│  └── for each enabled rule matching trigger:
│      └── executor.execute(rule.action, ctx)
```

### Two-level rules

- **Project events** — stored in `SongsManager`, apply regardless of which song is active
- **Song events** — stored in each `Song`, apply only when that song is loaded

When a song is loaded, `EventEngine::loadRules()` merges both levels.

### Extensibility

New action types are added by:

1. Implementing `ActionExecutor` interface
2. Registering it with `EventEngine::registerExecutor()`

Currently implemented: `MidiActionExecutor` (sends raw MIDI bytes via `MidiOutputManager`).

### Current triggers

| Trigger       | Fired when                             |
| ------------- | -------------------------------------- |
| `song.loaded` | A song is loaded into the audio engine |

## MIDI Output

`MidiOutputManager` wraps JUCE's `MidiOutput` API:

- Lists available MIDI output devices (name + identifier)
- Opens devices lazily on first use and caches connections
- Sends raw MIDI byte arrays to named devices

## Mode System

Two application modes control what the user can do:

| Mode     | Can play | Can edit | Track freezing |
| -------- | -------- | -------- | -------------- |
| **Live** | Yes      | No       | Enabled        |
| **Edit** | Yes      | Yes      | Disabled       |

`ModeManager` enforces transitions: switching to Edit mode is rejected while audio is playing. Edit-only commands are rejected in Live mode at the WebSocket layer.

## Frontend Architecture

### Tech stack

- **Tauri** — desktop shell (Rust-based, lighter than Electron)
- **React 19** + **TypeScript** — UI framework
- **Vite** — build tool
- **shadcn/ui** — component library (Radix + Tailwind)

### Structure

```
frontend/src/
├── features/               Feature-based modules
│   ├── audio-settings/     Audio device configuration dialog
│   ├── events/             Programmable events dialog
│   ├── navbar/             Top navigation bar + mode toggle
│   ├── projects/           Project browser dialog
│   ├── sequencer/          Track timeline with canvas rendering
│   ├── transport/          Play/stop/BPM controls
│   └── websocket/          Connection dialog
├── pages/                  Top-level pages
│   ├── ConnectionPage      WebSocket connection screen
│   └── HomePage             Main DAW interface
└── shared/
    ├── contexts/           React contexts (WebSocket, Project, Mode, Events, etc.)
    ├── models/             TypeScript interfaces mirroring backend models
    ├── services/           WebSocket command definitions
    └── shadcn/             UI components (button, dialog, slider, etc.)
```

### Data flow

The frontend is a thin client. All state lives in the backend.

```
User action → WebSocket command → Backend processes → Broadcast event → React context updates → UI re-renders
```

Key contexts:

- **WebSocketProvider** — connection management, send/receive
- **ProjectProvider** — active song, playhead position, transport state
- **ModeProvider** — Live/Edit mode (follows backend)
- **EventsProvider** — event rules CRUD
- **AudioDevicesProvider** — audio device listing and selection

### Playhead interpolation

The backend broadcasts playhead position at ~10 Hz. The frontend interpolates between broadcasts using `requestAnimationFrame` for smooth 60 fps timeline animation (see `useInterpolatedPlayhead` hook).

## Dependencies

### Backend

| Dependency    | Version | Purpose                                    |
| ------------- | ------- | ------------------------------------------ |
| JUCE          | 7.x     | Audio framework (git submodule)            |
| Crow          | 1.2.0   | WebSocket/HTTP server (CMake FetchContent) |
| ASIO          | 1.30.2  | Async I/O for Crow (CMake FetchContent)    |
| nlohmann/json | 3.11.3  | JSON serialization (CMake FetchContent)    |
| ALSA          | system  | Linux audio driver                         |
| JACK          | system  | Optional Linux audio driver                |

### Frontend

| Dependency          | Version | Purpose                            |
| ------------------- | ------- | ---------------------------------- |
| Tauri               | 2.x     | Desktop shell                      |
| React               | 19.x    | UI framework                       |
| TypeScript          | 5.x     | Type safety                        |
| Vite                | 7.x     | Build tool                         |
| Tailwind CSS        | 4.x     | Styling                            |
| @tabler/icons-react | —       | Icon set                           |
| Radix UI            | —       | Accessible primitives (via shadcn) |
