#include <juce_core/juce_core.h>
#include "model/loop.hpp"
#include "services/loop-manager.hpp"

class LoopManagerTests : public juce::UnitTest {
 public:
  LoopManagerTests() : juce::UnitTest("LoopManager Tests") {}

  void runTest() override {
    beginTest("No active loop when empty");
    testNoLoopsEmpty();

    beginTest("Loop activates when playhead enters it");
    testActivatesOnEntry();

    beginTest("Loop-back at end position");
    testLoopBack();

    beginTest("Cancel deactivates active loop");
    testCancel();

    beginTest("Exit returns end position");
    testExit();

    beginTest("Overlapping loop queued, activates after cancel");
    testOverlapping();

    beginTest("Loop activates when playback starts inside it");
    testActivatesWhenStartingInside();

    beginTest("Reset clears state");
    testReset();
  }

 private:
  void testNoLoopsEmpty() {
    LoopManager lm;
    expect(!lm.hasActiveLoop());
    auto result = lm.checkPosition(0.0, 1.0);
    expect(!result.jumpTo.has_value());
    expect(!result.activated);
  }

  void testActivatesOnEntry() {
    LoopManager lm;
    Loop loop;
    loop.id = "a";
    loop.start = 2.0;
    loop.end = 5.0;
    lm.setLoops({loop});

    auto r1 = lm.checkPosition(0.0, 1.9);
    expect(!r1.activated);
    expect(!lm.hasActiveLoop());

    auto r2 = lm.checkPosition(1.9, 2.1);
    expect(r2.activated, "Should activate when crossing loop.start");
    expect(lm.hasActiveLoop());
    expect(!r2.jumpTo.has_value(), "No jump on entry");
  }

  void testLoopBack() {
    LoopManager lm;
    Loop loop;
    loop.id = "a";
    loop.start = 2.0;
    loop.end = 5.0;
    lm.setLoops({loop});
    lm.checkPosition(1.9, 2.1);

    auto r = lm.checkPosition(4.9, 5.1);
    expect(r.jumpTo.has_value(), "Should return jump position");
    expectWithinAbsoluteError(*r.jumpTo, 2.0, 0.001, "Jump to loop start");
  }

  void testCancel() {
    LoopManager lm;
    Loop loop;
    loop.id = "a";
    loop.start = 2.0;
    loop.end = 5.0;
    lm.setLoops({loop});
    lm.checkPosition(1.9, 2.1);
    expect(lm.hasActiveLoop());

    lm.cancelActiveLoop();
    expect(!lm.hasActiveLoop(), "Should be deactivated after cancel");
  }

  void testExit() {
    LoopManager lm;
    Loop loop;
    loop.id = "a";
    loop.start = 2.0;
    loop.end = 5.0;
    lm.setLoops({loop});
    lm.checkPosition(1.9, 2.1);

    auto endPos = lm.exitActiveLoop();
    expect(endPos.has_value(), "Exit should return end position");
    expectWithinAbsoluteError(*endPos, 5.0, 0.001);
    expect(!lm.hasActiveLoop());
  }

  void testOverlapping() {
    LoopManager lm;
    Loop a; a.id = "a"; a.start = 1.0; a.end = 6.0;
    Loop b; b.id = "b"; b.start = 3.0; b.end = 8.0;
    lm.setLoops({a, b});

    lm.checkPosition(0.9, 1.1);
    expect(lm.hasActiveLoop());
    expect(lm.getActiveLoop()->id == "a");

    lm.checkPosition(2.9, 3.1);  // enter b's range while a is active
    expect(lm.getActiveLoop()->id == "a", "a stays active");

    lm.cancelActiveLoop();
    lm.checkPosition(3.1, 3.2);  // pos is inside b → promote b
    expect(lm.hasActiveLoop());
    expect(lm.getActiveLoop()->id == "b", "b promoted after a cancelled");
  }

  void testActivatesWhenStartingInside() {
    LoopManager lm;
    Loop loop;
    loop.id = "a";
    loop.start = 2.0;
    loop.end = 8.0;
    lm.setLoops({loop});

    // Playback starts at 5.0, already inside the loop — should activate immediately
    auto r = lm.checkPosition(5.0, 5.1);
    expect(r.activated, "Should activate when playback starts inside loop");
    expect(lm.hasActiveLoop());
    expect(!r.jumpTo.has_value(), "No jump on initial activation");
  }

  void testReset() {
    LoopManager lm;
    Loop loop; loop.id = "a"; loop.start = 1.0; loop.end = 5.0;
    lm.setLoops({loop});
    lm.checkPosition(0.9, 1.1);
    expect(lm.hasActiveLoop());

    lm.reset();
    expect(!lm.hasActiveLoop());
    auto r = lm.checkPosition(0.9, 1.1);
    expect(r.activated, "Loop can activate again after reset");
  }
};

static LoopManagerTests loopManagerTests;
