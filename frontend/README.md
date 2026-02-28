# DAW Frontend

Desktop application built with Tauri, React 19, and TypeScript.

## Setup

```bash
pnpm install
```

## Development

```bash
pnpm dev          # Tauri dev mode with hot reload
```

The backend must be running on `ws://localhost:8080/ws` for the app to function.

## Scripts

| Command | Description |
|---|---|
| `pnpm dev` | Start dev server with hot reload |
| `pnpm build` | Production build |
| `pnpm build:linux` | Linux package |
| `pnpm build:win` | Windows package |
| `pnpm build:mac` | macOS package |
| `pnpm lint` | ESLint check |
| `pnpm format` | Prettier format |

Type checking:
```bash
npx tsc --noEmit
```

## Project Structure

```
src/
├── features/           Feature-based modules
│   ├── audio-settings/ Audio device configuration
│   ├── events/         Programmable events (MIDI triggers)
│   ├── navbar/         Navigation bar + mode toggle
│   ├── projects/       Project browser
│   ├── sequencer/      Track timeline (canvas rendering)
│   ├── transport/      Playback controls
│   └── websocket/      Connection management
├── pages/              Top-level pages
├── shared/
│   ├── contexts/       React contexts (WebSocket, Project, Mode, etc.)
│   ├── models/         TypeScript interfaces
│   ├── services/       WebSocket command definitions
│   └── shadcn/         UI component library
└── App.tsx             Root component
```

See [ARCHITECTURE.md](../ARCHITECTURE.md) for detailed architecture documentation.
