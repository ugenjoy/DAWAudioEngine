#include <juce_core/juce_core.h>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "events/action-executor.hpp"
#include "events/transport-action-executor.hpp"

class TransportPlayExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "transport.play"; }
  void execute(const EventAction&, AppContext& ctx) override {
    ctx.getAudioEngine().play();
    juce::Logger::writeToLog("[TransportPlayExecutor] play");
  }
};

class TransportPauseExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "transport.pause"; }
  void execute(const EventAction&, AppContext& ctx) override {
    ctx.getAudioEngine().pause();
    juce::Logger::writeToLog("[TransportPauseExecutor] pause");
  }
};

class TransportStopExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "transport.stop"; }
  void execute(const EventAction&, AppContext& ctx) override {
    ctx.getAudioEngine().stop();
    juce::Logger::writeToLog("[TransportStopExecutor] stop");
  }
};

std::vector<std::unique_ptr<ActionExecutor>> createTransportActionExecutors() {
  std::vector<std::unique_ptr<ActionExecutor>> v;
  v.push_back(std::make_unique<TransportPlayExecutor>());
  v.push_back(std::make_unique<TransportPauseExecutor>());
  v.push_back(std::make_unique<TransportStopExecutor>());
  return v;
}
