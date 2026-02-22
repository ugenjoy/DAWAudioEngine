#pragma once

#include "commands/command.hpp"

#include <string>

/**
 * Command to list available audio devices.
 */
class ListDevicesCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Command to set the audio input/output device.
 */
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

/**
 * Command to list available audio input channels.
 */
class ListInputsCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Command to set the input channel for a track.
 */
class SetTrackInputCommand : public Command {
 public:
  SetTrackInputCommand(std::string trackId, int inputChannel, bool stereo);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  int inputChannel;
  bool stereo;
};

/**
 * Command to toggle monitoring on a track.
 */
class SetTrackMonitoringCommand : public Command {
 public:
  SetTrackMonitoringCommand(std::string trackId, bool monitoring);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  bool monitoring;
};
