#pragma once

#include "commands/command.hpp"

/**
 * Command to start playback.
 */
class PlayCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
  std::string getName() const override { return "transport.play"; }
};

/**
 * Command to pause playback.
 */
class PauseCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
  std::string getName() const override { return "transport.pause"; }
};

/**
 * Command to stop playback and reset position.
 */
class StopCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
  std::string getName() const override { return "transport.stop"; }
};
