#pragma once

class AudioEngineCore;
class SongsManager;
class ProjectManager;
class WebSocketServer;
class ModeManager;
class CommandProcessor;
class EventEngine;
class MidiOutputManager;

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
             MidiOutputManager& midiOutputManager)
      : audioEngine(audioEngine),
        songsManager(songsManager),
        projectManager(projectManager),
        wsServer(wsServer),
        modeManager(modeManager),
        eventEngine(eventEngine),
        midiOutputManager(midiOutputManager) {}

  AudioEngineCore& getAudioEngine() { return audioEngine; }
  SongsManager& getSongsManager() { return songsManager; }
  ProjectManager& getProjectManager() { return projectManager; }
  WebSocketServer& getWebSocketServer() { return wsServer; }
  ModeManager& getModeManager() { return modeManager; }
  EventEngine& getEventEngine() { return eventEngine; }
  MidiOutputManager& getMidiOutputManager() { return midiOutputManager; }

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
  CommandProcessor* commandProcessor = nullptr;
};
