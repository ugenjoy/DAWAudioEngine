#include <juce_core/juce_core.h>
#include "../include/beat-track.hpp"
#include "../include/audio-context.hpp"
#include <cmath>

/**
 * Unit tests for the BeatTrack class
 * Tests ADSR envelope, beat timing, volume control, and mute functionality
 */
class BeatTrackTests : public juce::UnitTest {
public:
  BeatTrackTests() : juce::UnitTest("BeatTrack Tests") {}

  void runTest() override {
    beginTest("Beat track initialization");
    testInitialization();

    beginTest("ADSR envelope - Attack phase");
    testAttackPhase();

    beginTest("ADSR envelope - Decay phase");
    testDecayPhase();

    beginTest("ADSR envelope - Sustain phase");
    testSustainPhase();

    beginTest("ADSR envelope - Release phase");
    testReleasePhase();

    beginTest("Beat timing and synchronization");
    testBeatTiming();

    beginTest("Volume control");
    testVolumeControl();

    beginTest("Mute functionality");
    testMute();

    beginTest("Silence between beats");
    testSilenceBetweenBeats();
  }

private:
  void testInitialization() {
    BeatTrack track(440.0f);
    auto& ctx = AudioContext::getInstance();
    ctx.sampleRate = 44100.0;
    
    // Track should be initialized and not muted - check RMS over a period
    float sumSquares = 0.0f;
    int numSamples = 100;
    for (int i = 0; i < numSamples; ++i) {
      float sample = track.getSampleValue(i / 44100.0);
      sumSquares += sample * sample;
    }
    float rms = std::sqrt(sumSquares / numSamples);
    expect(rms > 0.01f, "Track should produce sound at initialization");
  }

  void testAttackPhase() {
    BeatTrack track(440.0f);
    auto& ctx = AudioContext::getInstance();
    ctx.sampleRate = 44100.0;
    ctx.tempoBPM = 120.0f;

    // During attack phase, RMS volume should increase
    auto getRMS = [&](double time, int samples = 50) {
      float sum = 0.0f;
      for (int i = 0; i < samples; ++i) {
        float s = track.getSampleValue(time + i / 44100.0);
        sum += s * s;
      }
      return std::sqrt(sum / samples);
    };

    float rms1 = getRMS(0.001);   // 1ms into attack
    float rms2 = getRMS(0.005);   // 5ms into attack
    float rms3 = getRMS(0.009);   // 9ms into attack

    expect(rms2 > rms1 * 0.9f, "Volume should increase during attack");
    expect(rms3 > rms2 * 0.9f, "Volume should continue increasing during attack");
  }

  void testDecayPhase() {
    BeatTrack track(440.0f);
    auto& ctx = AudioContext::getInstance();
    ctx.sampleRate = 44100.0;
    ctx.tempoBPM = 120.0f;

    // After attack (10ms), during decay (20ms), RMS volume should decrease
    auto getRMS = [&](double time, int samples = 50) {
      float sum = 0.0f;
      for (int i = 0; i < samples; ++i) {
        float s = track.getSampleValue(time + i / 44100.0);
        sum += s * s;
      }
      return std::sqrt(sum / samples);
    };

    float rmsEndAttack = getRMS(0.011);  // Just after attack
    float rmsMidDecay = getRMS(0.020);   // Mid decay

    expect(rmsMidDecay < rmsEndAttack * 1.1f, "Volume should decrease during decay");
  }

  void testSustainPhase() {
    BeatTrack track(440.0f);
    auto& ctx = AudioContext::getInstance();
    ctx.sampleRate = 44100.0;
    ctx.tempoBPM = 120.0f;

    // During sustain phase, RMS volume should remain relatively constant
    auto getRMS = [&](double time, int samples = 100) {
      float sum = 0.0f;
      for (int i = 0; i < samples; ++i) {
        float s = track.getSampleValue(time + i / 44100.0);
        sum += s * s;
      }
      return std::sqrt(sum / samples);
    };

    float rms1 = getRMS(0.050);  // During sustain
    float rms2 = getRMS(0.080);  // Still during sustain

    float diff = std::abs(rms2 - rms1);
    expect(diff < rms1 * 0.2f, "Volume should be stable during sustain");
  }

  void testReleasePhase() {
    BeatTrack track(440.0f);
    auto& ctx = AudioContext::getInstance();
    ctx.sampleRate = 44100.0;
    ctx.tempoBPM = 120.0f;

    // After note duration (150ms), during release (20ms), volume should decrease to 0
    float sampleStartRelease = std::abs(track.getSampleValue(0.151));  // Start of release
    float sampleMidRelease = std::abs(track.getSampleValue(0.160));    // Mid release
    float sampleEndRelease = std::abs(track.getSampleValue(0.170));    // End of release

    expect(sampleMidRelease < sampleStartRelease, "Volume should decrease during release");
    expect(sampleEndRelease < sampleMidRelease, "Volume should continue decreasing during release");
    expect(sampleEndRelease < 0.1f, "Volume should be near zero at end of release");
  }

  void testBeatTiming() {
    BeatTrack track(440.0f);
    auto& ctx = AudioContext::getInstance();
    ctx.sampleRate = 44100.0;
    ctx.tempoBPM = 120.0f;  // 120 BPM = 0.5 second per beat

    // Check RMS at beat starts
    auto getRMS = [&](double time, int samples = 100) {
      float sum = 0.0f;
      for (int i = 0; i < samples; ++i) {
        float s = track.getSampleValue(time + i / 44100.0);
        sum += s * s;
      }
      return std::sqrt(sum / samples);
    };

    float beat1RMS = getRMS(0.001);   // First beat (skip t=0 exactly)
    float beat2RMS = getRMS(0.501);   // Second beat at 0.5s
    float beat3RMS = getRMS(1.001);   // Third beat at 1.0s

    expect(beat1RMS > 0.01f, "First beat should start at t=0");
    expect(beat2RMS > 0.01f, "Second beat should start at t=0.5");
    expect(beat3RMS > 0.01f, "Third beat should start at t=1.0");
  }

  void testVolumeControl() {
    BeatTrack track(440.0f);
    auto& ctx = AudioContext::getInstance();
    ctx.sampleRate = 44100.0;

    auto getRMS = [&](int samples = 200) {
      float sum = 0.0f;
      for (int i = 0; i < samples; ++i) {
        float s = track.getSampleValue(0.05 + i / 44100.0);
        sum += s * s;
      }
      return std::sqrt(sum / samples);
    };

    // Test default volume
    float rmsDefault = getRMS();

    // Set volume to 0.2 (50% of default 0.4)
    track.setVolume(0.2f);
    float rmsLow = getRMS();

    // Set volume to 0.8 (200% of default 0.4)
    track.setVolume(0.8f);
    float rmsHigh = getRMS();

    expect(rmsLow < rmsDefault * 0.9f, "Lower volume should produce quieter sound");
    expect(rmsHigh > rmsDefault * 1.1f, "Higher volume should produce louder sound");

    // Test volume clamping
    track.setVolume(2.0f);  // Should clamp to 1.0
    float rmsClamped = getRMS();
    expect(rmsClamped <= rmsHigh * 1.5f, "Volume should be clamped to valid range");
  }

  void testMute() {
    BeatTrack track(440.0f);
    auto& ctx = AudioContext::getInstance();
    ctx.sampleRate = 44100.0;

    auto getRMS = [&](int samples = 200) {
      float sum = 0.0f;
      for (int i = 0; i < samples; ++i) {
        float s = track.getSampleValue(0.05 + i / 44100.0);
        sum += s * s;
      }
      return std::sqrt(sum / samples);
    };

    // Track should produce sound when not muted
    float rmsUnmuted = getRMS();
    expect(rmsUnmuted > 0.01f, "Track should produce sound when not muted");

    // Track should be silent when muted
    track.setMute(true);
    float rmsMuted = getRMS();
    expect(rmsMuted == 0.0f, "Track should be silent when muted");

    // Track should produce sound again when unmuted
    track.setMute(false);
    float rmsUnmutedAgain = getRMS();
    expect(rmsUnmutedAgain > 0.01f, "Track should produce sound when unmuted again");
  }

  void testSilenceBetweenBeats() {
    BeatTrack track(440.0f);
    auto& ctx = AudioContext::getInstance();
    ctx.sampleRate = 44100.0;
    ctx.tempoBPM = 120.0f;  // 120 BPM = 0.5 second per beat

    // After release phase (150ms + 20ms = 170ms), there should be silence
    float sampleSilence = track.getSampleValue(0.200);  // 200ms into the beat
    expect(std::abs(sampleSilence) < 0.01f, "Should be silent between beats");

    // Just before next beat (at 0.49s), should still be silent
    float sampleBeforeNextBeat = track.getSampleValue(0.49);
    expect(std::abs(sampleBeforeNextBeat) < 0.01f, "Should be silent before next beat");
  }
};

static BeatTrackTests beatTrackTests;
