#pragma once

#include <string>

#include "commands/command.hpp"

class ListDevicesCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class SetAudioDeviceCommand : public Command {
 public:
  SetAudioDeviceCommand(std::string deviceType, std::string outputDevice,
                        std::string inputDevice, double sampleRate,
                        int bufferSize);
  void execute(AppContext& ctx) override;

 private:
  std::string deviceType;
  std::string outputDevice;
  std::string inputDevice;
  double sampleRate;
  int bufferSize;
};

class ListInputsCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class SetTrackInputCommand : public Command {
 public:
  SetTrackInputCommand(std::string trackId, int inputChannel, bool stereo);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  int inputChannel;
  bool stereo;
};

class SetTrackMonitoringCommand : public Command {
 public:
  SetTrackMonitoringCommand(std::string trackId, bool monitoring);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  bool monitoring;
};

class SetTrackMuteCommand : public Command {
 public:
  SetTrackMuteCommand(std::string trackId, bool mute);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  bool mute;
};

class SetTrackSoloCommand : public Command {
 public:
  SetTrackSoloCommand(std::string trackId, bool solo);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  bool solo;
};

class SetTrackVolumeCommand : public Command {
 public:
  SetTrackVolumeCommand(std::string trackId, float volume);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  float volume;
};

class AddTrackCommand : public Command {
 public:
  AddTrackCommand(std::string name);
  void execute(AppContext& ctx) override;

 private:
  std::string name;
};

class RemoveTrackCommand : public Command {
 public:
  RemoveTrackCommand(std::string trackId);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
};

class RenameTrackCommand : public Command {
 public:
  RenameTrackCommand(std::string trackId, std::string name);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  std::string name;
};

class ReorderTrackCommand : public Command {
 public:
  ReorderTrackCommand(std::string trackId, int index);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  int index;
};

class SetTrackColorCommand : public Command {
 public:
  SetTrackColorCommand(std::string trackId, int color);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  int color;
};
