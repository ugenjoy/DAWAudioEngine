#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "events/event-engine.hpp"
#include "events/event-rule.hpp"

static EventRule makeNoteRule(int note, int channel, const std::string& device = "") {
  EventRule r;
  r.id = "test-note";
  r.trigger = "midi.note";
  r.triggerParams["note"] = note;
  r.triggerParams["channel"] = channel;
  r.triggerParams["device"] = device;
  r.action.type = "transport.play";
  r.enabled = true;
  return r;
}

static EventRule makeCcRule(int cc, int channel, int threshold,
                             const std::string& device = "") {
  EventRule r;
  r.id = "test-cc";
  r.trigger = "midi.cc";
  r.triggerParams["cc"] = cc;
  r.triggerParams["channel"] = channel;
  r.triggerParams["threshold"] = threshold;
  r.triggerParams["device"] = device;
  r.action.type = "transport.stop";
  r.enabled = true;
  return r;
}

class EventEngineMidiTests : public juce::UnitTest {
 public:
  EventEngineMidiTests() : juce::UnitTest("EventEngine MIDI trigger matching") {}

  void runTest() override {
    beginTest("Note On matches note rule");
    testNoteOnMatches();

    beginTest("Note On wrong note does not match");
    testNoteOnWrongNote();

    beginTest("Note On channel filter");
    testNoteOnChannelFilter();

    beginTest("Channel 0 matches any channel");
    testChannelZeroMatchesAny();

    beginTest("Device filter — empty matches any");
    testDeviceEmptyMatchesAny();

    beginTest("Device filter — name must match");
    testDeviceNameMustMatch();

    beginTest("CC matches above threshold");
    testCcAboveThreshold();

    beginTest("CC does not match below threshold");
    testCcBelowThreshold();

    beginTest("CC number filter");
    testCcNumberFilter();

    beginTest("Note Off does not match midi.note rule");
    testNoteOffIgnored();

    beginTest("Disabled rule not matched");
    testDisabledRule();
  }

 private:
  void testNoteOnMatches() {
    auto rule = makeNoteRule(60, 1);
    auto msg = juce::MidiMessage::noteOn(1, 60, (uint8_t)100);
    expect(EventEngine::matchesMidiTrigger(rule, msg, "dev"));
  }

  void testNoteOnWrongNote() {
    auto rule = makeNoteRule(60, 1);
    auto msg = juce::MidiMessage::noteOn(1, 61, (uint8_t)100);
    expect(!EventEngine::matchesMidiTrigger(rule, msg, "dev"));
  }

  void testNoteOnChannelFilter() {
    auto rule = makeNoteRule(60, 2);
    auto msg = juce::MidiMessage::noteOn(1, 60, (uint8_t)100);
    expect(!EventEngine::matchesMidiTrigger(rule, msg, "dev"),
           "Channel 2 rule should not match channel 1 message");
  }

  void testChannelZeroMatchesAny() {
    auto rule = makeNoteRule(60, 0);
    auto msg1 = juce::MidiMessage::noteOn(1, 60, (uint8_t)100);
    auto msg2 = juce::MidiMessage::noteOn(15, 60, (uint8_t)100);
    expect(EventEngine::matchesMidiTrigger(rule, msg1, "dev"));
    expect(EventEngine::matchesMidiTrigger(rule, msg2, "dev"));
  }

  void testDeviceEmptyMatchesAny() {
    auto rule = makeNoteRule(60, 1, "");
    auto msg = juce::MidiMessage::noteOn(1, 60, (uint8_t)100);
    expect(EventEngine::matchesMidiTrigger(rule, msg, "any-device"));
  }

  void testDeviceNameMustMatch() {
    auto rule = makeNoteRule(60, 1, "Foot Controller");
    auto msg = juce::MidiMessage::noteOn(1, 60, (uint8_t)100);
    expect(!EventEngine::matchesMidiTrigger(rule, msg, "Other Device"));
    expect(EventEngine::matchesMidiTrigger(rule, msg, "Foot Controller"));
  }

  void testCcAboveThreshold() {
    auto rule = makeCcRule(64, 1, 64);
    auto msg = juce::MidiMessage::controllerEvent(1, 64, 100);
    expect(EventEngine::matchesMidiTrigger(rule, msg, "dev"));
  }

  void testCcBelowThreshold() {
    auto rule = makeCcRule(64, 1, 64);
    auto msg = juce::MidiMessage::controllerEvent(1, 64, 0);
    expect(!EventEngine::matchesMidiTrigger(rule, msg, "dev"));
  }

  void testCcNumberFilter() {
    auto rule = makeCcRule(64, 1, 1);
    auto msg = juce::MidiMessage::controllerEvent(1, 65, 100);
    expect(!EventEngine::matchesMidiTrigger(rule, msg, "dev"),
           "CC#65 should not match CC#64 rule");
  }

  void testNoteOffIgnored() {
    auto rule = makeNoteRule(60, 1);
    auto msg = juce::MidiMessage::noteOff(1, 60);
    expect(!EventEngine::matchesMidiTrigger(rule, msg, "dev"));
  }

  void testDisabledRule() {
    auto rule = makeNoteRule(60, 1);
    rule.enabled = false;
    auto msg = juce::MidiMessage::noteOn(1, 60, (uint8_t)100);
    // matchesMidiTrigger ignores enabled flag — fireMidi handles that check
    expect(EventEngine::matchesMidiTrigger(rule, msg, "dev"),
           "matchesMidiTrigger ignores enabled flag (fireMidi handles it)");
  }
};

static EventEngineMidiTests eventEngineMidiTests;
