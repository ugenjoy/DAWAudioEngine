#pragma once

#include <cassert>
#include "services/live-setlist-manager.hpp"
#include "services/loop-manager.hpp"
#include "services/project-manager.hpp"
#include "services/session.hpp"

class AudioEngineCore;
class WebSocketServer;
class ModeManager;
class CommandProcessor;
class EventEngine;
class MidiOutputManager;
class SongPreloader;
class MidiInputManager;

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
             ProjectManager& projectManager,
             WebSocketServer& wsServer,
             ModeManager& modeManager,
             EventEngine& eventEngine,
             MidiOutputManager& midiOutputManager,
             SongPreloader& songPreloader,
             LiveSetlistManager& liveSetlistManager,
             LoopManager& loopManager,
             MidiInputManager& midiInputManager,
             Session& session)
      : audioEngine(audioEngine),
        projectManager(projectManager),
        wsServer(wsServer),
        modeManager(modeManager),
        eventEngine(eventEngine),
        midiOutputManager(midiOutputManager),
        songPreloader(songPreloader),
        liveSetlistManager(liveSetlistManager),
        loopManager(loopManager),
        midiInputManager(midiInputManager),
        session(session) {}

  AudioEngineCore& getAudioEngine() { return audioEngine; }
  SongsManager& getSongsManager() {
    assert(projectManager.getCurrentProjectRef() &&
           "getSongsManager() called with no project loaded");
    return projectManager.getCurrentProjectRef()->songs();
  }
  ProjectManager& getProjectManager() { return projectManager; }
  WebSocketServer& getWebSocketServer() { return wsServer; }
  ModeManager& getModeManager() { return modeManager; }
  EventEngine& getEventEngine() { return eventEngine; }
  MidiOutputManager& getMidiOutputManager() { return midiOutputManager; }
  SongPreloader& getSongPreloader() { return songPreloader; }
  SetlistManager& getSetlistManager() {
    assert(projectManager.getCurrentProjectRef() &&
           "getSetlistManager() called with no project loaded");
    return projectManager.getCurrentProjectRef()->setlists();
  }
  LiveSetlistManager& getLiveSetlistManager() { return liveSetlistManager; }
  LoopManager& getLoopManager() { return loopManager; }
  MidiInputManager& getMidiInputManager() { return midiInputManager; }
  Session& getSession() { return session; }

  /** Injected after construction to avoid circular dependency. */
  void setCommandProcessor(CommandProcessor* processor) {
    commandProcessor = processor;
  }
  CommandProcessor* getCommandProcessor() { return commandProcessor; }

 private:
  AudioEngineCore& audioEngine;
  ProjectManager& projectManager;
  WebSocketServer& wsServer;
  ModeManager& modeManager;
  EventEngine& eventEngine;
  MidiOutputManager& midiOutputManager;
  SongPreloader& songPreloader;
  LiveSetlistManager& liveSetlistManager;
  LoopManager& loopManager;
  MidiInputManager& midiInputManager;
  Session& session;
  CommandProcessor* commandProcessor = nullptr;
};
