#pragma once

#include "services/loop-manager.hpp"
#include "services/session.hpp"

class AudioEngineCore;
class WebSocketServer;
class CommandProcessor;
class EventEngine;
class MidiOutputManager;
class SongPreloader;
class MidiInputManager;

/**
 * Application context providing access to all core services.
 * Passed to commands for executing operations across different subsystems.
 *
 * Project/song/setlist data is accessed via getSession().getProject()->*().
 */
class AppContext {
 public:
  AppContext(AudioEngineCore& audioEngine,
             WebSocketServer& wsServer,
             EventEngine& eventEngine,
             MidiOutputManager& midiOutputManager,
             SongPreloader& songPreloader,
             LoopManager& loopManager,
             MidiInputManager& midiInputManager,
             Session& session)
      : audioEngine(audioEngine),
        wsServer(wsServer),
        eventEngine(eventEngine),
        midiOutputManager(midiOutputManager),
        songPreloader(songPreloader),
        loopManager(loopManager),
        midiInputManager(midiInputManager),
        session(session) {}

  AudioEngineCore& getAudioEngine() { return audioEngine; }
  WebSocketServer& getWebSocketServer() { return wsServer; }
  EventEngine& getEventEngine() { return eventEngine; }
  MidiOutputManager& getMidiOutputManager() { return midiOutputManager; }
  SongPreloader& getSongPreloader() { return songPreloader; }
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
  WebSocketServer& wsServer;
  EventEngine& eventEngine;
  MidiOutputManager& midiOutputManager;
  SongPreloader& songPreloader;
  LoopManager& loopManager;
  MidiInputManager& midiInputManager;
  Session& session;
  CommandProcessor* commandProcessor = nullptr;
};
