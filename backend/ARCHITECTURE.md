# DAW Backend Architecture

## Overview

The backend is a JUCE console application with a Crow WebSocket server. Commands arrive as JSON messages, are dispatched through a queue, and executed against the application context.

## Core Components

### Session (`services/session.hpp`)

Single source of truth for "what the app is doing now". Owns the current `Project` and tracks the current activity.

```
Session
├── Project (owned, unique_ptr)
│   ├── SongsManager (owned)
│   ├── SetlistManager (owned)
│   └── projectEventRules
├── activity: None | Editing | Performing
├── editingSongId (when Editing)
├── performingSetlistId + performingIndex (when Performing)
└── Listener (broadcast session.changed on every transition)
```

**Activity transitions:**
- `startEditing(songId)` → loads audio, activity = Editing
- `startPerforming(setlistId)` → preloads songs, activity = Performing
- `stopActivity()` → activity = None
- `onSongEndReached()` → auto-advance or stop based on SetlistTransition

**Access pattern:** `ctx.getSession().getProject()->songs()`

### Project (`model/project.hpp`)

Owns everything that belongs to one `.dawproj` folder on disk.

```cpp
static std::unique_ptr<Project> createNew(name, parentDir);
static std::unique_ptr<Project> loadFromDisk(path);
static Project readMetadata(path);  // lightweight, no managers

SongsManager& songs();
SetlistManager& setlists();
bool save();
```

### ProjectRepository (`services/project-repository.hpp`)

Free functions for disk scanning:
```cpp
namespace ProjectRepository {
  std::string getDefaultProjectsDirectory();
  std::vector<Project> scan(directory);  // metadata snapshots
}
```

### AppContext (`app-context.hpp`)

Passed to all commands. Access pattern after the Session refactor:
```
ctx.getSession()                          → Session&
ctx.getSession().getProject()             → Project*
ctx.getSession().getProject()->songs()    → SongsManager&
ctx.getSession().getProject()->setlists() → SetlistManager&
ctx.getAudioEngine()                      → AudioEngineCore&
ctx.getWebSocketServer()                  → WebSocketServer&
ctx.getEventEngine()                      → EventEngine&
ctx.getSongPreloader()                    → SongPreloader&
ctx.getLoopManager()                      → LoopManager&
```

### WebSocketServer

- Receives JSON `{ "action": "...", ...payload }` from clients
- Filters edit-only commands via `session.isEdit()` (`REGISTER_EDIT_COMMAND`)
- Dispatches to `CommandQueue` → `CommandProcessor`
- Has HTTP route for audio file upload

### Commands

Commands auto-register at static init time via `REGISTER_COMMAND` / `REGISTER_EDIT_COMMAND` macros.

| Namespace | Commands |
|-----------|---------|
| `session.*` | openProject, closeProject, startEditing, startPerforming, stopActivity, advance, previous, goTo, get |
| `project.*` | create, save, getLoaded, list |
| `song.*` | create, rename, reorder, delete, setEndPosition |
| `setlist.*` | list, create, update, delete |
| `track.*` | add, remove, rename, reorder, setColor, setInput, setMute, setSolo, setVolume, setMonitoring |
| `transport.*` | play, pause, stop, setPlayheadPosition, setCursorPosition, setMasterVolume |
| `event.*` | list, add, remove, update |
| `loop.*` | add, remove, update, cancel, exit |
| `clip.*` | add, move, remove |
| `midi.*` | listOutputs, listInputs, send |
| `marker.*` | list, add, remove, update |

### Events

`session.changed` is the primary broadcast event. Sent whenever Session transitions (project open/close, activity change, performing navigation). Payload:
```json
{
  "hasProject": true,
  "project": { "id", "name", "path", "lastModified", "songs", "setlists", "events" },
  "activity": "editing" | "performing" | "none",
  "editing": { "songId", "song" } | null,
  "performing": { "setlistId", "currentIndex", "song" } | null,
  "playback": { "isPlaying", "playheadPosition", "cursorPosition", "masterVolume" }
}
```

Other events (kept): `song.loaded`, `song.unloaded`, `song.loading`, `project.songsUpdated`, `setlist.listUpdated`.

## Audio Engine

`AudioEngineCore` (JUCE + JACK/ALSA). RT audio callback runs on a separate thread. `getActiveSong()` returns a cached pointer — invariant: `audioEngine.getActiveSong() == session.getFocusedSong()`.

## File Layout

```
backend/
  include/
    app-context.hpp
    model/          project.hpp, song.hpp, setlist.hpp, ...
    services/       session.hpp, project-repository.hpp, songs-manager.hpp, ...
    commands/       session-commands.hpp, project-commands.hpp, ...
    events/         event-engine.hpp, event-rule.hpp, ...
    audio/          audio-engine-core.hpp, ...
    websocket/      websocket-server.hpp
  src/
    main.cpp
    model/          project.cpp, ...
    services/       session.cpp, ...
    commands/       session-commands.cpp, project-commands.cpp, ...
    events/         ...
    audio/          ...
    websocket/      websocket-server.cpp
```
