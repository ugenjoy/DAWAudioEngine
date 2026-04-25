#pragma once

#include <string>

#include "commands/command.hpp"

class CreateProjectCommand : public Command {
 public:
  explicit CreateProjectCommand(std::string name);

  void execute(AppContext& ctx) override;

 private:
  std::string name;
};

class LoadProjectCommand : public Command {
 public:
  explicit LoadProjectCommand(std::string projectPath);

  void execute(AppContext& ctx) override;

 private:
  std::string projectPath;
};

class SaveProjectCommand : public Command {
 public:
  explicit SaveProjectCommand(std::string projectPath);

  void execute(AppContext& ctx) override;

 private:
  std::string projectPath;
};

class GetLoadedProjectCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class ListProjectsCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class LoadSongCommand : public Command {
 public:
  explicit LoadSongCommand(std::string uuid);

  void execute(AppContext& ctx) override;

 private:
  std::string uuid;
};
