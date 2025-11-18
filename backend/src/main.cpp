#include "audio-engine-core.hpp"
#include "songs-manager.hpp"
#include "websocket-server.hpp"

class AudioEngineApplication : public juce::JUCEApplication,
                               public juce::Timer {
 public:
  const juce::String getApplicationName() override {
    return "DAW Audio Engine";
  }

  const juce::String getApplicationVersion() override { return "1.0.0"; }

  void initialise(const juce::String& commandLine) override {
    juce::Logger::writeToLog("=== DAW Audio Engine - Starting ===");

    // Create audio engine
    audioEngine = std::make_unique<AudioEngineCore>();

    // Create songsManager
    songsManager = std::make_unique<SongsManager>();

    auto song1 = std::make_unique<Song>();
    auto track1 = std::make_unique<BeatTrack>(1000.0f);
    song1->setTempo(100);
    song1->addTrack(std::move(track1));

    auto song2 = std::make_unique<Song>();
    auto track2 = std::make_unique<BeatTrack>(600.0f);
    song2->setTempo(60);
    song2->addTrack(std::move(track2));

    audioEngine->loadSong(song1.get());

    songsManager->addSong(std::move(song1));
    songsManager->addSong(std::move(song2));

    juce::Logger::writeToLog("Audio engine created. You should hear a beat.");

    // Start WebSocket server
    wsServer = std::make_unique<WebSocketServer>(audioEngine.get(),
                                                 songsManager.get());
    wsServer->start(8080);

    juce::Logger::writeToLog("Press Ctrl+C to quit.");

    // Monitor server status every 500ms
    startTimer(500);
  }

  void shutdown() override {
    juce::Logger::writeToLog("=== Stopping WebSocket server ===");
    wsServer.reset();

    juce::Logger::writeToLog("=== Stopping audio engine ===");
    audioEngine.reset();
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
  std::unique_ptr<SongsManager> songsManager;
  std::unique_ptr<AudioEngineCore> audioEngine;
  std::unique_ptr<WebSocketServer> wsServer;
};

// Entry point
START_JUCE_APPLICATION(AudioEngineApplication)