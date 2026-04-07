#include <juce_core/juce_core.h>

#include "app-context.hpp"
#include "events/action-executor.hpp"
#include "events/setlist-action-executor.hpp"
#include "services/live-setlist-manager.hpp"

class SetlistNextExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "setlist.next"; }
  void execute(const EventAction&, AppContext& ctx) override {
    bool ok = ctx.getLiveSetlistManager().advance(ctx);
    juce::Logger::writeToLog(
        juce::String("[SetlistNextExecutor] advance: ") + (ok ? "OK" : "no-op"));
  }
};

class SetlistPrevExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "setlist.prev"; }
  void execute(const EventAction&, AppContext& ctx) override {
    bool ok = ctx.getLiveSetlistManager().previous(ctx);
    juce::Logger::writeToLog(
        juce::String("[SetlistPrevExecutor] previous: ") + (ok ? "OK" : "no-op"));
  }
};

std::vector<std::unique_ptr<ActionExecutor>> createSetlistActionExecutors() {
  std::vector<std::unique_ptr<ActionExecutor>> v;
  v.push_back(std::make_unique<SetlistNextExecutor>());
  v.push_back(std::make_unique<SetlistPrevExecutor>());
  return v;
}
