#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "commands/command-processor.hpp"
#include "commands/command-queue.hpp"
#include "events/event-engine.hpp"
#include "events/midi-action-executor.hpp"
#include "services/midi-output-manager.hpp"
#include "services/mode-manager.hpp"
#include "services/song-preloader.hpp"
#include "services/project-manager.hpp"
#include "services/songs-manager.hpp"
#include "websocket/websocket-server.hpp"

class AudioEngineApplication : public juce::JUCEApplication,
                               public juce::Timer {
 public:
  const juce::String getApplicationName() override {
    return "DAW Audio Engine";
  }

  const juce::String getApplicationVersion() override { return "1.0.0"; }

  void initialise(const juce::String& commandLine) override {
    juce::ignoreUnused(commandLine);
    juce::Logger::writeToLog("=== DAW Audio Engine - Starting ===");

    // Create core components
    audioEngine = std::make_unique<AudioEngineCore>();
    songsManager = std::make_unique<SongsManager>();
    projectManager = std::make_unique<ProjectManager>();
    modeManager = std::make_unique<ModeManager>();
    midiOutputManager = std::make_unique<MidiOutputManager>();
    songPreloader = std::make_unique<SongPreloader>();

    // Create and configure event engine
    eventEngine = std::make_unique<EventEngine>();
    eventEngine->registerExecutor(createMidiActionExecutor());

    // Connect ModeManager to AudioEngineCore playback state
    modeManager->setPlayingStateProvider(
        [this]() { return audioEngine->isPlaying(); });

    // Create command factory (auto-registered commands)
    commandFactory = CommandFactory::create();

    // Create command queue
    commandQueue = std::make_unique<CommandQueue>();

    // Create WebSocket server with mode filtering support
    wsServer = std::make_unique<WebSocketServer>(*commandQueue, *commandFactory,
                                                 *modeManager, 8080);

    audioEngine->setWebSocketServer(wsServer.get());

    // Create application context
    appContext = std::make_unique<AppContext>(
        *audioEngine, *songsManager, *projectManager, *wsServer, *modeManager,
        *eventEngine, *midiOutputManager, *songPreloader);

    // Inject AppContext into WebSocket server for HTTP routes
    wsServer->setAppContext(appContext.get());

    // Create command processor and inject into context (avoids circular dep)
    commandProcessor =
        std::make_unique<CommandProcessor>(*commandQueue, *appContext);
    appContext->setCommandProcessor(commandProcessor.get());
    commandProcessor->startProcessing();
    juce::Logger::writeToLog("Command processor started");

    // Start WebSocket server
    wsServer->startAsync();

    juce::Logger::writeToLog("WebSocket server starting on port 8080");
    juce::Logger::writeToLog("Press Ctrl+C to quit.");

    // Monitor server status every 500ms
    startTimer(500);
  }

  void shutdown() override {
    juce::Logger::writeToLog("=== Stopping WebSocket server ===");
    wsServer.reset();

    juce::Logger::writeToLog("=== Stopping command processor ===");
    if (commandProcessor) {
      commandProcessor->stopProcessing();
    }
    commandProcessor.reset();
    commandQueue.reset();

    juce::Logger::writeToLog("=== Stopping audio engine ===");
    appContext.reset();
    audioEngine.reset();
    songsManager.reset();
    projectManager.reset();
    modeManager.reset();
    midiOutputManager.reset();
    songPreloader.reset();
    eventEngine.reset();
    commandFactory.reset();
  }

  void timerCallback() override {
    // Check if WebSocket server thread has exited (e.g., due to Ctrl+C)
    if (wsServer && wsServer->hasExited()) {
      juce::Logger::writeToLog(
          "=== Server thread exited, quitting application ===");
      quit();
    }
  }

 private:
  std::unique_ptr<AudioEngineCore> audioEngine;
  std::unique_ptr<SongsManager> songsManager;
  std::unique_ptr<ProjectManager> projectManager;
  std::unique_ptr<ModeManager> modeManager;
  std::unique_ptr<MidiOutputManager> midiOutputManager;
  std::unique_ptr<SongPreloader> songPreloader;
  std::unique_ptr<EventEngine> eventEngine;
  std::unique_ptr<AppContext> appContext;
  std::unique_ptr<CommandFactory> commandFactory;
  std::unique_ptr<CommandQueue> commandQueue;
  std::unique_ptr<CommandProcessor> commandProcessor;
  std::unique_ptr<WebSocketServer> wsServer;
};

// Entry point
START_JUCE_APPLICATION(AudioEngineApplication)
