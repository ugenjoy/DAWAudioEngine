#include "commands/command-factory.hpp"
#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>

std::unordered_map<std::string, CommandFactory::Creator>&
CommandFactory::getRegistry() {
  // Meyer's singleton - thread-safe in C++11+
  static std::unordered_map<std::string, Creator> registry;
  return registry;
}

std::unique_ptr<CommandFactory> CommandFactory::create() {
  auto factory = std::make_unique<CommandFactory>();

  // Copy all auto-registered commands from the global registry
  for (const auto& [action, creator] : getRegistry()) {
    factory->registerCommand(action, creator);
  }

  juce::Logger::writeToLog("CommandFactory: " +
                           juce::String(factory->getRegisteredActions().size()) +
                           " commands registered");

  return factory;
}

void CommandFactory::registerCommand(const std::string& action,
                                     Creator creator) {
  creators[action] = std::move(creator);
}

CommandPtr CommandFactory::create(const std::string& action,
                                  const nlohmann::json& payload) const {
  auto it = creators.find(action);
  if (it == creators.end()) {
    return nullptr;
  }
  return it->second(payload);
}

bool CommandFactory::hasAction(const std::string& action) const {
  return creators.find(action) != creators.end();
}

std::vector<std::string> CommandFactory::getRegisteredActions() const {
  std::vector<std::string> actions;
  actions.reserve(creators.size());
  for (const auto& [action, _] : creators) {
    actions.push_back(action);
  }
  return actions;
}
