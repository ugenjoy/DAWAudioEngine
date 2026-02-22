#pragma once

#include <string>
#include "events/event-rule.hpp"

class AppContext;

/**
 * Abstract interface for executing a specific action type.
 * Implement this for each action type (midi.send, osc.send, http.request, etc.)
 */
class ActionExecutor {
 public:
  virtual ~ActionExecutor() = default;

  /**
   * Returns the action type this executor handles (e.g. "midi.send").
   */
  virtual std::string getActionType() const = 0;

  /**
   * Execute the given action using the application context.
   * @param action The action to execute (type is guaranteed to match getActionType())
   * @param ctx Application context providing access to all services
   */
  virtual void execute(const EventAction& action, AppContext& ctx) = 0;
};
