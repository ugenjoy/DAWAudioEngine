#include "events/event-engine.hpp"
#include "events/event-rule.hpp"
#include <juce_core/juce_core.h>

static EventRule makePositionRule(double position, bool enabled = true) {
  EventRule r;
  r.id = "test-pos";
  r.trigger = "position";
  r.triggerParams["position"] = position;
  r.action.type = "transport.stop";
  r.enabled = enabled;
  return r;
}

class EventEnginePositionTests : public juce::UnitTest {
 public:
  EventEnginePositionTests() : juce::UnitTest("EventEngine position trigger matching") {}

  void runTest() override {
    beginTest("Fires when position is in ]prev, current]");
    expect(EventEngine::matchesPositionTrigger(makePositionRule(2.5), 2.0, 2.6));

    beginTest("Fires at exact currentPos boundary");
    expect(EventEngine::matchesPositionTrigger(makePositionRule(2.5), 2.0, 2.5));

    beginTest("Does not fire when prevPos == triggerPos (already passed)");
    expect(!EventEngine::matchesPositionTrigger(makePositionRule(2.5), 2.5, 2.6));

    beginTest("Does not fire when trigger is ahead of window");
    expect(!EventEngine::matchesPositionTrigger(makePositionRule(3.0), 2.0, 2.6));

    beginTest("Does not fire when trigger is behind window");
    expect(!EventEngine::matchesPositionTrigger(makePositionRule(1.0), 2.0, 2.6));

    beginTest("matchesPositionTrigger ignores enabled flag (firePosition handles it)");
    expect(EventEngine::matchesPositionTrigger(makePositionRule(2.5, false), 2.0, 2.6));

    beginTest("Does not match non-position rule");
    auto r = makePositionRule(2.5);
    r.trigger = "song.loaded";
    expect(!EventEngine::matchesPositionTrigger(r, 2.0, 2.6));
  }
};

static EventEnginePositionTests eventEnginePositionTests;
