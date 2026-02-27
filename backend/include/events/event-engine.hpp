#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "events/action-executor.hpp"
#include "events/event-rule.hpp"

class AppContext;

/**
 * Central event dispatch engine.
 * Stores event rules (project-level and song-level) and dispatches
 * trigger events to the appropriate ActionExecutors.
 */
class EventEngine {
 public:
  EventEngine() = default;
  ~EventEngine() = default;

  /**
   * Register an executor for a specific action type.
   * Must be called before firing events.
   */
  void registerExecutor(std::unique_ptr<ActionExecutor> executor);

  /**
   * Load rules for the current context.
   * Replaces any previously loaded rules.
   * @param projectRules Rules defined at the project level
   * @param songRules Rules defined at the current song level
   */
  void loadRules(const std::vector<EventRule>& projectRules,
                 const std::vector<EventRule>& songRules);

  /**
   * Clear all loaded rules (e.g. when unloading a project).
   */
  void clearRules();

  /**
   * Fire a trigger, executing all matching enabled rules.
   * @param trigger The trigger name (e.g. "song.loaded")
   * @param ctx Application context passed to executors
   */
  void fire(const std::string& trigger, AppContext& ctx);

 private:
  std::unordered_map<std::string, std::unique_ptr<ActionExecutor>> executors;
  std::vector<EventRule> rules;  // Combined project + song rules
};
