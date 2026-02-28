#pragma once

#include "commands/command.hpp"

class SetTempoCommand : public Command {
 public:
  explicit SetTempoCommand(float tempo);
  void execute(AppContext& ctx) override;

 private:
  float tempo;
};

class SetMetronomeMuteCommand : public Command {
 public:
  explicit SetMetronomeMuteCommand(bool mute);
  void execute(AppContext& ctx) override;

 private:
  bool mute;
};
