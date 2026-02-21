#pragma once

#include "commands/command.hpp"

/**
 * Command to start playback.
 */
class PlayCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Command to pause playback.
 */
class PauseCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Command to stop playback and reset position.
 */
class StopCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Command to set playback position.
 */
class SetPositionCommand : public Command {
 public:
  explicit SetPositionCommand(double position);

  void execute(AppContext& ctx) override;

 private:
  double position;
};
