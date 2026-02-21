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
 * Command to set playhead position.
 */
class SetPlayheadPositionCommand : public Command {
 public:
  explicit SetPlayheadPositionCommand(double position);

  void execute(AppContext& ctx) override;

 private:
  double position;
};

/**
 * Command to set cursor position.
 */
class SetCursorPositionCommand : public Command {
 public:
  explicit SetCursorPositionCommand(double position);

  void execute(AppContext& ctx) override;

 private:
  double position;
};
