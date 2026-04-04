#pragma once

#include <optional>

#include "commands/command.hpp"

class SetTempoCommand : public Command {
 public:
  explicit SetTempoCommand(float tempo);
  void execute(AppContext& ctx) override;

 private:
  float tempo;
};

class CreateSongCommand : public Command {
 public:
  explicit CreateSongCommand(std::string name);
  void execute(AppContext& ctx) override;

 private:
  std::string name;
};

class RenameSongCommand : public Command {
 public:
  RenameSongCommand(std::string uuid, std::string name);
  void execute(AppContext& ctx) override;

 private:
  std::string uuid;
  std::string name;
};

class ReorderSongCommand : public Command {
 public:
  ReorderSongCommand(std::string uuid, int index);
  void execute(AppContext& ctx) override;

 private:
  std::string uuid;
  int index;
};

class SetMetronomeMuteCommand : public Command {
 public:
  explicit SetMetronomeMuteCommand(bool mute);
  void execute(AppContext& ctx) override;

 private:
  bool mute;
};

class SetEndPositionCommand : public Command {
 public:
  SetEndPositionCommand(std::string uuid, std::optional<double> pos);
  void execute(AppContext& ctx) override;

 private:
  std::string uuid;
  std::optional<double> pos;
};

class DeleteSongCommand : public Command {
 public:
  explicit DeleteSongCommand(std::string uuid);
  void execute(AppContext& ctx) override;

 private:
  std::string uuid;
};
