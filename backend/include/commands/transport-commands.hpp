#pragma once

#include "commands/command.hpp"

class PlayCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class PauseCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class StopCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class SetPlayheadPositionCommand : public Command {
 public:
  explicit SetPlayheadPositionCommand(double position);

  void execute(AppContext& ctx) override;

 private:
  double position;
};

class SetCursorPositionCommand : public Command {
 public:
  explicit SetCursorPositionCommand(double position);

  void execute(AppContext& ctx) override;

 private:
  double position;
};

class SetMasterVolumeCommand : public Command {
 public:
  explicit SetMasterVolumeCommand(float volume);

  void execute(AppContext& ctx) override;

 private:
  float volume;
};
