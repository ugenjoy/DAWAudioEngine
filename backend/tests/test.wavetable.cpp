#include <juce_core/juce_core.h>
#include "../include/wave-table.hpp"
#include <cmath>

/**
 * Unit tests for the WaveTable class
 * Tests waveform generation, phase wrapping, and interpolation
 */
class WaveTableTests : public juce::UnitTest {
public:
  WaveTableTests() : juce::UnitTest("WaveTable Tests") {}

  void runTest() override {
    beginTest("Sine wave generation");
    testSineWave();

    beginTest("Square wave generation");
    testSquareWave();

    beginTest("Saw wave generation");
    testSawWave();

    beginTest("Triangle wave generation");
    testTriangleWave();

    beginTest("Phase wrapping");
    testPhaseWrapping();

    beginTest("Linear interpolation");
    testInterpolation();

    beginTest("Fast vs interpolated lookup");
    testFastVsInterpolated();
  }

private:
  void testSineWave() {
    WaveTable waveTable(WaveTable::SINE, 2048);
    float pi = juce::MathConstants<float>::pi;

    // Test at key points
    float sample0 = waveTable.getSample(0.0f);
    expect(std::abs(sample0) < 0.01f, "Sine at 0 should be ~0");

    float samplePi2 = waveTable.getSample(pi / 2.0f);
    expect(std::abs(samplePi2 - 1.0f) < 0.01f, "Sine at pi/2 should be ~1");

    float samplePi = waveTable.getSample(pi);
    expect(std::abs(samplePi) < 0.01f, "Sine at pi should be ~0");

    float sample3Pi2 = waveTable.getSample(3.0f * pi / 2.0f);
    expect(std::abs(sample3Pi2 + 1.0f) < 0.01f, "Sine at 3pi/2 should be ~-1");
  }

  void testSquareWave() {
    WaveTable waveTable(WaveTable::SQUARE, 2048);
    float pi = juce::MathConstants<float>::pi;

    // First half should be positive
    float sample1 = waveTable.getSample(pi / 2.0f);
    expect(sample1 > 0.9f, "Square wave first half should be ~1");

    // Second half should be negative
    float sample2 = waveTable.getSample(3.0f * pi / 2.0f);
    expect(sample2 < -0.9f, "Square wave second half should be ~-1");
  }

  void testSawWave() {
    WaveTable waveTable(WaveTable::SAW, 2048);
    float pi = juce::MathConstants<float>::pi;

    // Sawtooth should ramp from -1 to 1
    float sample0 = waveTable.getSample(0.0f);
    expect(std::abs(sample0 + 1.0f) < 0.1f, "Saw at 0 should be ~-1");

    float samplePi = waveTable.getSample(pi);
    expect(std::abs(samplePi) < 0.1f, "Saw at pi should be ~0");

    float sample2Pi = waveTable.getSample(2.0f * pi - 0.01f);
    expect(sample2Pi > 0.8f, "Saw at 2pi should be ~1");
  }

  void testTriangleWave() {
    WaveTable waveTable(WaveTable::TRIANGLE, 2048);
    float pi = juce::MathConstants<float>::pi;

    // Triangle should go -1 -> 1 -> -1
    float sample0 = waveTable.getSample(0.0f);
    expect(std::abs(sample0 + 1.0f) < 0.1f, "Triangle at 0 should be ~-1");

    float samplePi2 = waveTable.getSample(pi / 2.0f);
    expect(std::abs(samplePi2) < 0.1f, "Triangle at pi/2 should be ~0");

    float samplePi = waveTable.getSample(pi);
    expect(std::abs(samplePi - 1.0f) < 0.1f, "Triangle at pi should be ~1");
  }

  void testPhaseWrapping() {
    WaveTable waveTable(WaveTable::SINE, 2048);
    float pi = juce::MathConstants<float>::pi;

    // Test that phase wraps correctly
    float sample1 = waveTable.getSample(0.0f);
    float sample2 = waveTable.getSample(2.0f * pi);
    float sample3 = waveTable.getSample(4.0f * pi);

    expect(std::abs(sample1 - sample2) < 0.01f,
           "Phase should wrap at 2pi");
    expect(std::abs(sample1 - sample3) < 0.01f,
           "Phase should wrap at 4pi");

    // Test negative phase
    float sample4 = waveTable.getSample(-2.0f * pi);
    expect(std::abs(sample1 - sample4) < 0.01f,
           "Negative phase should wrap correctly");
  }

  void testInterpolation() {
    WaveTable waveTable(WaveTable::SINE, 128);  // Smaller table to see interpolation effect
    float pi = juce::MathConstants<float>::pi;

    // Test that interpolated values are smooth
    float prev = waveTable.getSample(0.0f);
    for (int i = 1; i < 100; ++i) {
      float phase = (2.0f * pi * i) / 100.0f;
      float current = waveTable.getSample(phase);
      
      // Check that changes are gradual (no huge jumps)
      float diff = std::abs(current - prev);
      expect(diff < 0.5f, "Interpolation should produce smooth values");
      
      prev = current;
    }
  }

  void testFastVsInterpolated() {
    WaveTable waveTable(WaveTable::SINE, 2048);
    float pi = juce::MathConstants<float>::pi;

    // Fast and interpolated should be similar for large tables
    for (int i = 0; i < 10; ++i) {
      float phase = (2.0f * pi * i) / 10.0f;
      float fast = waveTable.getSampleFast(phase);
      float interpolated = waveTable.getSample(phase);
      
      float diff = std::abs(fast - interpolated);
      expect(diff < 0.1f, "Fast and interpolated should be similar");
    }
  }
};

static WaveTableTests waveTableTests;
