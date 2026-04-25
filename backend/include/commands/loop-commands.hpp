#pragma once

#include "commands/command.hpp"

class LoopAddCommand : public Command {
 public:
  LoopAddCommand(double start, double end);
  void execute(AppContext& ctx) override;
 private:
  double start;
  double end;
};

class LoopRemoveCommand : public Command {
 public:
  explicit LoopRemoveCommand(std::string loopId);
  void execute(AppContext& ctx) override;
 private:
  std::string loopId;
};

class LoopUpdateCommand : public Command {
 public:
  LoopUpdateCommand(std::string loopId, double start, double end);
  void execute(AppContext& ctx) override;
 private:
  std::string loopId;
  double start;
  double end;
};

class LoopCancelCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class LoopExitCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};
