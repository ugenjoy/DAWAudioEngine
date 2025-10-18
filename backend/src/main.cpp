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

    juce::Logger::writeToLog("Audio engine created. You should ear a beat.");
    juce::Logger::writeToLog("Press Ctrl+C to quit.");

    // Prevent the app from closing
    startTimer(1000);
  }

  void shutdown() override {
    juce::Logger::writeToLog("=== Stoping audio engine ===");
    audioEngine.reset();
  }

  void timerCallback() override {
    // Does not do nothing, just for keeping the app alive
  }

 private:
  std::unique_ptr<AudioEngineCore> audioEngine;
};

// Input point
START_JUCE_APPLICATION(AudioEngineApplication)