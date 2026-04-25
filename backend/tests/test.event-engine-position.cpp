#include "events/event-engine.hpp"
#include "events/event-rule.hpp"
#include "model/marker.hpp"
#include <juce_core/juce_core.h>

static Marker makeMarker(const std::string& id, double position) {
  Marker m;
  m.id = id;
  m.name = id;
  m.position = position;
  return m;
}

static EventRule makePositionRule(const std::string& markerId, bool enabled = true) {
  EventRule r;
  r.id = "test-pos";
  r.trigger = "position";
  r.triggerParams["markerId"] = markerId;
  r.action.type = "transport.stop";
  r.enabled = enabled;
  return r;
}

class EventEnginePositionTests : public juce::UnitTest {
 public:
  EventEnginePositionTests() : juce::UnitTest("EventEngine position trigger matching") {}

  void runTest() override {
    EventEngine engine;
    engine.loadMarkers({ makeMarker("m1", 2.5), makeMarker("m2", 3.0), makeMarker("m3", 1.0) });

    beginTest("Fires when position is in ]prev, current]");
    expect(engine.matchesPositionTrigger(makePositionRule("m1"), 2.0, 2.6));

    beginTest("Fires at exact currentPos boundary");
    expect(engine.matchesPositionTrigger(makePositionRule("m1"), 2.0, 2.5));

    beginTest("Does not fire when prevPos == triggerPos (already passed)");
    expect(!engine.matchesPositionTrigger(makePositionRule("m1"), 2.5, 2.6));

    beginTest("Does not fire when trigger is ahead of window");
    expect(!engine.matchesPositionTrigger(makePositionRule("m2"), 2.0, 2.6));

    beginTest("Does not fire when trigger is behind window");
    expect(!engine.matchesPositionTrigger(makePositionRule("m3"), 2.0, 2.6));

    beginTest("matchesPositionTrigger ignores enabled flag (firePosition handles it)");
    expect(engine.matchesPositionTrigger(makePositionRule("m1", false), 2.0, 2.6));

    beginTest("Does not match non-position rule");
    auto r = makePositionRule("m1");
    r.trigger = "song.loaded";
    expect(!engine.matchesPositionTrigger(r, 2.0, 2.6));

    beginTest("Does not match when markerId is unknown");
    expect(!engine.matchesPositionTrigger(makePositionRule("unknown"), 2.0, 2.6));

    beginTest("Does not match when markerId is empty");
    EventRule emptyIdRule;
    emptyIdRule.trigger = "position";
    emptyIdRule.triggerParams["markerId"] = std::string("");
    expect(!engine.matchesPositionTrigger(emptyIdRule, 2.0, 2.6));
  }
};

static EventEnginePositionTests eventEnginePositionTests;
