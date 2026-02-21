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

 private:
  std::string projectPath;
};

/**
 * Command to get the currently loaded project info.
 * Broadcasts the current project path to all connected clients.
 */
class GetLoadedProjectCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Command to list all projects in the default projects directory.
 */
class ListProjectsCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Command to load a song of the project.
 */
class LoadSongCommand : public Command {
 public:
  explicit LoadSongCommand(std::string uuid);

  void execute(AppContext& ctx) override;

 private:
  std::string uuid;
};
