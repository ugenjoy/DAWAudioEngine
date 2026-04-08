#include <juce_core/juce_core.h>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "events/action-executor.hpp"
#include "events/event-engine.hpp"
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

class TransportSeekExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "transport.seekToPosition"; }
  void execute(const EventAction& action, AppContext& ctx) override {
    std::string markerId = action.params.value("markerId", std::string(""));
    if (markerId.empty()) {
      juce::Logger::writeToLog("[TransportSeekExecutor] Missing markerId");
      return;
    }
    auto pos = ctx.getEventEngine().resolveMarker(markerId);
    if (!pos.has_value()) {
      juce::Logger::writeToLog(
          "[TransportSeekExecutor] Marker '" + juce::String(markerId) +
          "' not found");
      return;
    }
    ctx.getAudioEngine().setPlayheadPosition(pos.value());
    juce::Logger::writeToLog(
        "[TransportSeekExecutor] Seeked to " +
        juce::String(pos.value(), 3) + "s");
  }
};

std::vector<std::unique_ptr<ActionExecutor>> createTransportActionExecutors() {
  std::vector<std::unique_ptr<ActionExecutor>> v;
  v.push_back(std::make_unique<TransportPlayExecutor>());
  v.push_back(std::make_unique<TransportPauseExecutor>());
  v.push_back(std::make_unique<TransportStopExecutor>());
  v.push_back(std::make_unique<TransportSeekExecutor>());
  return v;
}
