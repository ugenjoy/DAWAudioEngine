#include <juce_core/juce_core.h>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "events/action-executor.hpp"
#include "events/loop-action-executor.hpp"
#include "services/loop-manager.hpp"

class LoopCancelExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "loop.cancel"; }
  void execute(const EventAction&, AppContext& ctx) override {
    bool ok = ctx.getLoopManager().cancelActiveLoop();
    juce::Logger::writeToLog(
        juce::String("[LoopCancelExecutor] cancel: ") + (ok ? "OK" : "no active loop"));
  }
};

class LoopExitExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "loop.exit"; }
  void execute(const EventAction&, AppContext& ctx) override {
    auto endPos = ctx.getLoopManager().exitActiveLoop();
    if (endPos.has_value()) {
      ctx.getAudioEngine().setPlayheadPosition(*endPos);
      juce::Logger::writeToLog("[LoopExitExecutor] exit to " +
                               juce::String(*endPos));
    } else {
      juce::Logger::writeToLog("[LoopExitExecutor] no active loop");
    }
  }
};

std::vector<std::unique_ptr<ActionExecutor>> createLoopActionExecutors() {
  std::vector<std::unique_ptr<ActionExecutor>> v;
  v.push_back(std::make_unique<LoopCancelExecutor>());
  v.push_back(std::make_unique<LoopExitExecutor>());
  return v;
}
