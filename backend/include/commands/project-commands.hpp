#pragma once

#include <string>
#include "commands/command.hpp"

/**
 * Command to load a project from disk.
 */
class LoadProjectCommand : public Command {
 public:
  explicit LoadProjectCommand(std::string projectPath);

  void execute(AppContext& ctx) override;
  std::string getName() const override { return "project.load"; }

 private:
  std::string projectPath;
};

/**
 * Command to save the current project to disk.
 */
class SaveProjectCommand : public Command {
 public:
  explicit SaveProjectCommand(std::string projectPath);

  void execute(AppContext& ctx) override;
  std::string getName() const override { return "project.save"; }

 private:
  std::string projectPath;
};

/**
 * Command to get the currently loaded project info.
 * Broadcasts the current project path to all connected clients.
 */
class GetLoadedProjectCommand : public Command {
 public:
  GetLoadedProjectCommand() = default;

  void execute(AppContext& ctx) override;
  std::string getName() const override { return "project.getLoaded"; }
};
