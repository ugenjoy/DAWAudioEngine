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
   * Create a factory with all auto-registered commands.
   */
  static std::unique_ptr<CommandFactory> create();

  /**
   * Access the global registry (for auto-registration).
   * @internal Used by REGISTER_COMMAND macro
   */
  static std::unordered_map<std::string, Creator>& getRegistry();

  /**
   * Register a command creator for an action.
   * @param action The action string (e.g., "transport.play")
   * @param creator Function that creates the command from JSON payload
   */
  void registerCommand(const std::string& action, Creator creator);

  /**
   * Create a command from action string and payload.
   * @param action The action string
   * @param payload JSON payload with command parameters
   * @return Command pointer, or nullptr if action not registered
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

 private:
  std::unordered_map<std::string, Creator> creators;
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
 * Macro to auto-register a command.
 *
 * For simple commands (no parameters):
 *   REGISTER_COMMAND("transport.play", PlayCommand)
 *
 * The command class must have a default constructor.
 */
#define REGISTER_COMMAND(action, CommandClass)                    \
  static CommandRegistrar registrar_##CommandClass(               \
      action,                                                     \
      [](const nlohmann::json&) { return std::make_unique<CommandClass>(); })

/**
 * Macro to auto-register a command with custom creator.
 *
 * For commands that need to parse JSON payload:
 *   REGISTER_COMMAND_WITH_CREATOR("project.load", [](const json& payload) {
 *     return std::make_unique<LoadProjectCommand>(payload.value("path", ""));
 *   })
 */
#define REGISTER_COMMAND_WITH_CREATOR(action, creatorLambda) \
  static CommandRegistrar registrar_##__LINE__(action, creatorLambda)
