// main.cpp - DAW Audio Engine (Base simple JUCE)
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

// =============================================
// AudioEngineCore - Gestion audio simple
// =============================================
class AudioEngineCore : public juce::AudioAppComponent {
 public:
  AudioEngineCore();
  ~AudioEngineCore() override;

  // AudioAppComponent overrides
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void getNextAudioBlock(
      const juce::AudioSourceChannelInfo& bufferToFill) override;
  void releaseResources() override;

 private:
  // État simple
  bool playing = false;
  double currentPosition = 0.0;
  double sampleRate = 44100.0;

  // Générateur de test simple (métronome)
  float phase = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngineCore)
};

// =============================================
// AudioEngineCore Implementation
// =============================================
AudioEngineCore::AudioEngineCore() {
  // Configuration audio : 0 entrées, 2 sorties stéréo
  setAudioChannels(0, 2);
}

AudioEngineCore::~AudioEngineCore() {
  shutdownAudio();
}

void AudioEngineCore::prepareToPlay(int samplesPerBlockExpected,
                                    double sampleRate) {
  this->sampleRate = sampleRate;

  juce::Logger::writeToLog("Audio initialisé:");
  juce::Logger::writeToLog(
      "- Taille buffer: " + juce::String(samplesPerBlockExpected) + " samples");
  juce::Logger::writeToLog("- Sample rate: " + juce::String(sampleRate) +
                           " Hz");
  juce::Logger::writeToLog("- Prêt à jouer!");

  // Démarrer automatiquement pour le test
  playing = true;
}

void AudioEngineCore::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill) {
  auto* buffer = bufferToFill.buffer;
  auto numSamples = bufferToFill.numSamples;

  if (!playing) {
    buffer->clear();
    return;
  }

  // Générateur de métronome simple (bip à 1Hz)
  for (int sample = 0; sample < numSamples; ++sample) {
    // Bip toutes les secondes
    float frequency = (std::fmod(currentPosition, 1.0) < 0.1) ? 1000.0f : 0.0f;
    float sampleValue = 0.1f * std::sin(phase);

    // Appliquer sur les deux canaux stéréo
    for (int channel = 0; channel < buffer->getNumChannels(); ++channel) {
      buffer->setSample(channel, sample, sampleValue);
    }

    // Avancer la phase
    phase +=
        2.0f * juce::MathConstants<float>::pi * frequency / (float)sampleRate;
    if (phase >= 2.0f * juce::MathConstants<float>::pi)
      phase -= 2.0f * juce::MathConstants<float>::pi;
  }

  // Mise à jour du temps
  currentPosition += (double)numSamples / sampleRate;

  // Log toutes les 4 secondes
  if (std::fmod(currentPosition, 4.0) < (double)numSamples / sampleRate) {
    juce::Logger::writeToLog("Position: " + juce::String(currentPosition, 2) +
                             "s");
  }
}

void AudioEngineCore::releaseResources() {
  juce::Logger::writeToLog("Ressources audio libérées");
}

// =============================================
// Application principale
// =============================================
class AudioEngineApplication : public juce::JUCEApplication,
                               public juce::Timer {
 public:
  const juce::String getApplicationName() override {
    return "DAW Audio Engine";
  }
  const juce::String getApplicationVersion() override { return "1.0.0"; }

  void initialise(const juce::String& commandLine) override {
    juce::Logger::writeToLog("=== DAW Audio Engine - Démarrage ===");

    // Créer le moteur audio
    audioEngine = std::make_unique<AudioEngineCore>();

    juce::Logger::writeToLog(
        "Moteur audio créé. Vous devriez entendre un métronome.");
    juce::Logger::writeToLog("Appuyez sur Ctrl+C pour quitter.");

    // Garder l'application vivante
    startTimer(1000);  // Timer pour éviter que l'app se ferme
  }

  void shutdown() override {
    juce::Logger::writeToLog("=== Arrêt du moteur audio ===");
    audioEngine.reset();
  }

  void timerCallback() override {
    // Ne fait rien, juste pour garder l'app active
  }

 private:
  std::unique_ptr<AudioEngineCore> audioEngine;
};

// Point d'entrée
START_JUCE_APPLICATION(AudioEngineApplication)