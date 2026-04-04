#pragma once

#include "services/setlist-manager.hpp"
#include "services/live-setlist-manager.hpp"

class AudioEngineCore;
class SongsManager;
class ProjectManager;
class WebSocketServer;
class ModeManager;
class CommandProcessor;
class EventEngine;
class MidiOutputManager;
class SongPreloader;

/**
 * Application context providing access to all core services.
 * Passed to commands for executing operations across different subsystems.
 *
 * This class does not own the services - it holds references to services
 * owned by the main application.
 */
class AppContext {
 public:
  AppContext(AudioEngineCore& audioEngine,
             SongsManager& songsManager,
             ProjectManager& projectManager,
             WebSocketServer& wsServer,
             ModeManager& modeManager,
             EventEngine& eventEngine,
             MidiOutputManager& midiOutputManager,
             SongPreloader& songPreloader,
             SetlistManager& setlistManager,
             LiveSetlistManager& liveSetlistManager)
      : audioEngine(audioEngine),
        songsManager(songsManager),
        projectManager(projectManager),
        wsServer(wsServer),
        modeManager(modeManager),
        eventEngine(eventEngine),
        midiOutputManager(midiOutputManager),
        songPreloader(songPreloader),
        setlistManager(setlistManager),
        liveSetlistManager(liveSetlistManager) {}

  AudioEngineCore& getAudioEngine() { return audioEngine; }
  SongsManager& getSongsManager() { return songsManager; }
  ProjectManager& getProjectManager() { return projectManager; }
  WebSocketServer& getWebSocketServer() { return wsServer; }
  ModeManager& getModeManager() { return modeManager; }
  EventEngine& getEventEngine() { return eventEngine; }
  MidiOutputManager& getMidiOutputManager() { return midiOutputManager; }
  SongPreloader& getSongPreloader() { return songPreloader; }
  SetlistManager& getSetlistManager() { return setlistManager; }
  LiveSetlistManager& getLiveSetlistManager() { return liveSetlistManager; }

  /** Injected after construction to avoid circular dependency. */
  void setCommandProcessor(CommandProcessor* processor) {
    commandProcessor = processor;
  }
  CommandProcessor* getCommandProcessor() { return commandProcessor; }

 private:
  AudioEngineCore& audioEngine;
  SongsManager& songsManager;
  ProjectManager& projectManager;
  WebSocketServer& wsServer;
  ModeManager& modeManager;
  EventEngine& eventEngine;
  MidiOutputManager& midiOutputManager;
  SongPreloader& songPreloader;
  SetlistManager& setlistManager;
  LiveSetlistManager& liveSetlistManager;
  CommandProcessor* commandProcessor = nullptr;
};
