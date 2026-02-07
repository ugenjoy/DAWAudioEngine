#pragma once

class AudioEngineCore;
class SongsManager;
class ProjectManager;
class WebSocketServer;

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
             WebSocketServer& wsServer)
      : audioEngine(audioEngine),
        songsManager(songsManager),
        projectManager(projectManager),
        wsServer(wsServer) {}

  AudioEngineCore& getAudioEngine() { return audioEngine; }
  SongsManager& getSongsManager() { return songsManager; }
  ProjectManager& getProjectManager() { return projectManager; }
  WebSocketServer& getWebSocketServer() { return wsServer; }

 private:
  AudioEngineCore& audioEngine;
  SongsManager& songsManager;
  ProjectManager& projectManager;
  WebSocketServer& wsServer;
};
