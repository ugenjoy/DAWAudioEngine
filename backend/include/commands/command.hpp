#pragma once

#include <functional>
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
  /**
   * Callback type for sending a response to the requester only.
   * Set automatically by WebSocketServer before the command is queued.
   */
  using ReplyFn = std::function<void(const std::string&)>;

  virtual ~Command() = default;

  /**
   * Execute the command.
   * Called by CommandProcessor on the worker thread.
   * @param ctx Application context providing access to all services
   */
  virtual void execute(AppContext& ctx) = 0;

  /**
   * Get the action name used to register and create this command.
   * Set automatically by CommandFactory — no need to override in subclasses.
   */
  const std::string& getName() const { return actionName; }

  /** @internal Called by WebSocketServer to attach the requester's reply channel. */
  void setReply(ReplyFn fn) { replyFn = std::move(fn); }

 protected:
  /**
   * Send a response to the requester only (not broadcast).
   * No-op if the client disconnected before the command was executed.
   */
  void reply(const std::string& message) const {
    if (replyFn) replyFn(message);
  }

 private:
  std::string actionName;
  ReplyFn replyFn;
  friend class CommandFactory;
};

using CommandPtr = std::unique_ptr<Command>;
