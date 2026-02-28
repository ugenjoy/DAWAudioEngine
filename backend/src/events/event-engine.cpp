#include "events/event-engine.hpp"

#include <juce_core/juce_core.h>

void EventEngine::registerExecutor(std::unique_ptr<ActionExecutor> executor) {
  std::string type = executor->getActionType();
  executors[type] = std::move(executor);
}

void EventEngine::loadRules(const std::vector<EventRule>& projectRules,
                            const std::vector<EventRule>& songRules) {
  rules.clear();
  rules.insert(rules.end(), projectRules.begin(), projectRules.end());
  rules.insert(rules.end(), songRules.begin(), songRules.end());

  juce::Logger::writeToLog(
      "[EventEngine] Loaded " + juce::String((int)rules.size()) +
      " rule(s) (" + juce::String((int)projectRules.size()) +
      " project, " + juce::String((int)songRules.size()) + " song)");
}

void EventEngine::clearRules() {
  rules.clear();
}

void EventEngine::fire(const std::string& trigger, AppContext& ctx) {
  int fired = 0;
  for (const auto& rule : rules) {
    if (!rule.enabled || rule.trigger != trigger) continue;

    // Skip gracefully if no executor is registered for this action type.
    // This allows rules to reference future action types without crashing.
    auto it = executors.find(rule.action.type);
    if (it == executors.end()) {
      juce::Logger::writeToLog(
          "[EventEngine] No executor for action type: " +
          juce::String(rule.action.type));
      continue;
    }

    juce::Logger::writeToLog(
        "[EventEngine] Firing rule '" + juce::String(rule.id) +
        "' → " + juce::String(rule.action.type));
    it->second->execute(rule.action, ctx);
    ++fired;
  }

  if (fired > 0) {
    juce::Logger::writeToLog(
        "[EventEngine] Fired " + juce::String(fired) +
        " rule(s) for trigger '" + juce::String(trigger) + "'");
  }
}
