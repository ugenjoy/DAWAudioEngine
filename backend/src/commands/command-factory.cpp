#include "commands/command-factory.hpp"
#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>

std::unordered_map<std::string, CommandFactory::CommandRegistration>&
CommandFactory::getRegistry() {
  // Meyer's singleton - thread-safe in C++11+
  static std::unordered_map<std::string, CommandRegistration> registry;
  return registry;
}

std::unique_ptr<CommandFactory> CommandFactory::create() {
  auto factory = std::make_unique<CommandFactory>();

  juce::Logger::writeToLog("CommandFactory: " +
                           juce::String(factory->getRegisteredActions().size()) +
                           " commands registered");

  return factory;
}

CommandPtr CommandFactory::create(const std::string& action,
                                  const nlohmann::json& payload) const {
  const auto& registry = getRegistry();
  auto it = registry.find(action);
  if (it == registry.end()) {
    return nullptr;
  }

  CommandPtr cmd = it->second.creator(payload);
  if (cmd) {
    cmd->actionName = action;
  }
  return cmd;
}

bool CommandFactory::hasAction(const std::string& action) const {
  return getRegistry().count(action) > 0;
}

std::vector<std::string> CommandFactory::getRegisteredActions() const {
  const auto& registry = getRegistry();
  std::vector<std::string> actions;
  actions.reserve(registry.size());
  for (const auto& [action, _] : registry) {
    actions.push_back(action);
  }
  return actions;
}

bool CommandFactory::isEditOnly(const std::string& action) const {
  const auto& registry = getRegistry();
  auto it = registry.find(action);
  if (it == registry.end()) return false;
  return it->second.editOnly;
}
