#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "commands/command-processor.hpp"
#include "commands/command-queue.hpp"
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

    // Create command factory (auto-registered commands)
    commandFactory = CommandFactory::create();

    // Create WebSocket server (needed for AppContext)
    commandQueue = std::make_unique<CommandQueue>();
    wsServer = std::make_unique<WebSocketServer>(*commandQueue, *commandFactory,
                                                 8080);

    // Create application context (with all services including wsServer)
    appContext = std::make_unique<AppContext>(*audioEngine, *songsManager,
                                              *projectManager, *wsServer);

    // Create command processor
    commandProcessor =
        std::make_unique<CommandProcessor>(*commandQueue, *appContext);
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
  std::unique_ptr<AppContext> appContext;
  std::unique_ptr<CommandFactory> commandFactory;
  std::unique_ptr<CommandQueue> commandQueue;
  std::unique_ptr<CommandProcessor> commandProcessor;
  std::unique_ptr<WebSocketServer> wsServer;
};

// Entry point
START_JUCE_APPLICATION(AudioEngineApplication)
