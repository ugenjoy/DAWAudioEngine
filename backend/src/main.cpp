#include "audio-engine-core.hpp"

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

    juce::Logger::writeToLog("Audio engine created. You should hear a beat.");
    juce::Logger::writeToLog("Press Ctrl+C to quit.");

    // Keep the application running
    startTimer(1000);
  }

  void shutdown() override {
    juce::Logger::writeToLog("=== Stopping audio engine ===");
    audioEngine.reset();
  }

  void timerCallback() override {
    // Empty callback to keep the application alive
  }

 private:
  std::unique_ptr<AudioEngineCore> audioEngine;
};

// Entry point
START_JUCE_APPLICATION(AudioEngineApplication)