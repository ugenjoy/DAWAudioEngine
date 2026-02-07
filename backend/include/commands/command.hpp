#pragma once

#include <memory>
#include <string>

// Forward declaration
class AppContext;

/**
 * Base interface for all commands.
 * Commands encapsulate actions that can be queued and executed asynchronously.
 * This enables thread-safe communication between WebSocket and Audio threads.
 */
class Command {
 public:
  virtual ~Command() = default;

  /**
   * Execute the command.
   * Called by CommandProcessor on the worker thread.
   * @param ctx Application context providing access to all services
   */
  virtual void execute(AppContext& ctx) = 0;

  /**
   * Get the command name for logging/debugging.
   */
  virtual std::string getName() const = 0;
};

using CommandPtr = std::unique_ptr<Command>;
