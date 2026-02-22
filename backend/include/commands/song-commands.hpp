#pragma once

#include "commands/command.hpp"

/**
 * Command to set the tempo of the active song.
 */
class SetTempoCommand : public Command {
 public:
  explicit SetTempoCommand(float tempo);
  void execute(AppContext& ctx) override;

 private:
  float tempo;
};

/**
 * Command to set the metronome mute state of the active song.
 */
class SetMetronomeMuteCommand : public Command {
 public:
  explicit SetMetronomeMuteCommand(bool mute);
  void execute(AppContext& ctx) override;

 private:
  bool mute;
};
