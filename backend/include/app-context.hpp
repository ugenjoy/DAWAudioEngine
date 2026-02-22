#pragma once

class AudioEngineCore;
class SongsManager;
class ProjectManager;
class WebSocketServer;
class ModeManager;
class CommandProcessor;

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
             ModeManager& modeManager)
      : audioEngine(audioEngine),
        songsManager(songsManager),
        projectManager(projectManager),
        wsServer(wsServer),
        modeManager(modeManager) {}

  AudioEngineCore& getAudioEngine() { return audioEngine; }
  SongsManager& getSongsManager() { return songsManager; }
  ProjectManager& getProjectManager() { return projectManager; }
  WebSocketServer& getWebSocketServer() { return wsServer; }
  ModeManager& getModeManager() { return modeManager; }

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
  CommandProcessor* commandProcessor = nullptr;
};
