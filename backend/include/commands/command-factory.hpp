#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "commands/command.hpp"
#include <nlohmann/json_fwd.hpp>

/**
 * Factory for creating commands from action strings.
 *
 * Commands auto-register themselves using the REGISTER_COMMAND macro.
 * The factory uses a singleton registry that collects all registrations
 * at static initialization time.
 *
 * Usage in command files:
 *   REGISTER_COMMAND("transport.play", PlayCommand)
 *
 * Usage in application:
 *   auto factory = CommandFactory::create();  // Gets all registered commands
 *   auto cmd = factory->create("transport.play", {});
 */
class CommandFactory {
 public:
  /**
   * Creator function type.
   * Takes JSON payload, returns a Command unique_ptr.
   */
  using Creator = std::function<CommandPtr(const nlohmann::json&)>;

  CommandFactory() = default;
  ~CommandFactory() = default;

  /**
   * Create a factory instance. Logs registered command count.
   */
  static std::unique_ptr<CommandFactory> create();

  /**
   * Access the global registry (for auto-registration).
   * @internal Used by REGISTER_COMMAND macros
   */
  static std::unordered_map<std::string, Creator>& getRegistry();

  /**
   * Create a command from action string and payload.
   * Automatically sets the command's name to the action string.
   * @param action The action string (e.g., "transport.play")
   * @param payload JSON payload with command parameters
   * @return Command pointer, or nullptr if action not registered or payload invalid
   */
  CommandPtr create(const std::string& action,
                    const nlohmann::json& payload) const;

  /**
   * Check if an action is registered.
   */
  bool hasAction(const std::string& action) const;

  /**
   * Get list of registered actions.
   */
  std::vector<std::string> getRegisteredActions() const;
};

/**
 * Helper class for auto-registration at static initialization.
 * @internal Used by REGISTER_COMMAND macro
 */
class CommandRegistrar {
 public:
  CommandRegistrar(const std::string& action, CommandFactory::Creator creator) {
    CommandFactory::getRegistry()[action] = std::move(creator);
  }
};

/**
 * Macro to auto-register a command with a default constructor (no payload).
 *
 * Usage:
 *   REGISTER_COMMAND("transport.play", PlayCommand)
 */
#define REGISTER_COMMAND(action, CommandClass)       \
  static CommandRegistrar registrar_##CommandClass(  \
      action,                                        \
      [](const nlohmann::json&) { return std::make_unique<CommandClass>(); })

/**
 * Macro to auto-register a command with a custom creator (parses JSON payload).
 * The `name` parameter is used as a unique C++ identifier suffix — use the
 * command class name without the "Command" suffix (e.g., SetPosition).
 *
 * Usage:
 *   REGISTER_COMMAND_WITH_CREATOR("transport.setPosition", SetPosition,
 *       [](const nlohmann::json& payload) {
 *         return std::make_unique<SetPositionCommand>(
 *             payload.value("position", 0.0));
 *       })
 */
#define REGISTER_COMMAND_WITH_CREATOR(action, name, creatorLambda) \
  static CommandRegistrar registrar_##name(action, creatorLambda)
