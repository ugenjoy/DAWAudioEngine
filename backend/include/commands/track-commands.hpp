#pragma once

#include <string>

#include "commands/command.hpp"

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

/**
 * Command to set the mute state of a track.
 */
class SetTrackMuteCommand : public Command {
 public:
  SetTrackMuteCommand(std::string trackId, bool mute);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  bool mute;
};

/**
 * Command to set the solo state of a track.
 */
class SetTrackSoloCommand : public Command {
 public:
  SetTrackSoloCommand(std::string trackId, bool solo);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  bool solo;
};

/**
 * Command to set the volume of a track.
 */
class SetTrackVolumeCommand : public Command {
 public:
  SetTrackVolumeCommand(std::string trackId, float volume);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  float volume;
};
