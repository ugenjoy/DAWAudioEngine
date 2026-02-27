#pragma once

#include <string>

#include "commands/command.hpp"
#include "events/event-rule.hpp"

/**
 * Command to list all event rules (project + active song).
 * Edit-mode only.
 */
class EventListCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Command to add a new event rule.
 * Edit-mode only.
 * Payload: { "scope": "project"|"song", "trigger": "...", "action": {...} }
 */
class EventAddCommand : public Command {
 public:
  EventAddCommand(std::string scope, EventRule rule);
  void execute(AppContext& ctx) override;

 private:
  std::string scope;
  EventRule rule;
};

/**
 * Command to remove an event rule by id.
 * Edit-mode only.
 * Payload: { "scope": "project"|"song", "eventId": "..." }
 */
class EventRemoveCommand : public Command {
 public:
  EventRemoveCommand(std::string scope, std::string eventId);
  void execute(AppContext& ctx) override;

 private:
  std::string scope;
  std::string eventId;
};

/**
 * Command to update an existing event rule.
 * Edit-mode only.
 * Payload: { "scope": "project"|"song", "eventId": "...", "trigger"?, "action"?, "enabled"? }
 */
class EventUpdateCommand : public Command {
 public:
  EventUpdateCommand(std::string scope, EventRule updated);
  void execute(AppContext& ctx) override;

 private:
  std::string scope;
  EventRule updated;
};
