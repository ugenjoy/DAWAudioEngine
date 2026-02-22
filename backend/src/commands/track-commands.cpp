#include "commands/track-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "model/song.hpp"

void ListDevicesCommand::execute(AppContext& ctx) {
  nlohmann::json response;
  response["type"] = "response";
  response["event"] = "audio.devicesList";
  auto devices = ctx.getAudioEngine().getAvailableDevices();
  response["deviceTypes"] = devices["deviceTypes"];
  if (devices.contains("current")) {
    response["current"] = devices["current"];
  }
  reply(response.dump());
}

SetAudioDeviceCommand::SetAudioDeviceCommand(std::string deviceType,
                                             std::string outputDevice,
                                             std::string inputDevice,
                                             double sampleRate,
                                             int bufferSize)
    : deviceType(std::move(deviceType)),
      outputDevice(std::move(outputDevice)),
      inputDevice(std::move(inputDevice)),
      sampleRate(sampleRate),
      bufferSize(bufferSize) {}

void SetAudioDeviceCommand::execute(AppContext& ctx) {
  auto result = ctx.getAudioEngine().setAudioDevice(
      juce::String(deviceType), juce::String(outputDevice),
      juce::String(inputDevice), sampleRate, bufferSize);

  nlohmann::json response;
  response["type"] = "broadcast";
  response["event"] = "audio.deviceChanged";
  response["current"] = ctx.getAudioEngine().getCurrentDeviceInfo();

  if (result.isNotEmpty()) {
    response["error"] = result.toStdString();
  }

  ctx.getWebSocketServer().broadcast(response.dump());

  // Also broadcast updated input list since channels may have changed
  nlohmann::json inputsMsg;
  inputsMsg["type"] = "broadcast";
  inputsMsg["event"] = "audio.inputsList";
  inputsMsg["inputs"] = ctx.getAudioEngine().getAvailableInputs();
  ctx.getWebSocketServer().broadcast(inputsMsg.dump());
}

void ListInputsCommand::execute(AppContext& ctx) {
  nlohmann::json response;
  response["type"] = "response";
  response["event"] = "audio.inputsList";
  response["inputs"] = ctx.getAudioEngine().getAvailableInputs();
  reply(response.dump());
}

SetTrackInputCommand::SetTrackInputCommand(std::string trackId,
                                           int inputChannel, bool stereo)
    : trackId(std::move(trackId)),
      inputChannel(inputChannel),
      stereo(stereo) {}

void SetTrackInputCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* track = song->getTracksManager()->findTrackById(trackId);
  if (!track) return;

  track->setInputChannel(inputChannel);
  track->setInputStereo(stereo);

  ctx.getAudioEngine().rebuildMonitoredChannelMask();

  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "track.inputChanged";
  broadcast["trackId"] = trackId;
  broadcast["inputChannel"] = inputChannel;
  broadcast["stereo"] = stereo;
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

SetTrackMonitoringCommand::SetTrackMonitoringCommand(std::string trackId,
                                                     bool monitoring)
    : trackId(std::move(trackId)), monitoring(monitoring) {}

void SetTrackMonitoringCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* track = song->getTracksManager()->findTrackById(trackId);
  if (!track) return;

  bool wasMonitoring = track->isMonitoring();
  if (wasMonitoring == monitoring) return;

  track->setMonitoring(monitoring);

  if (monitoring) {
    ctx.getAudioEngine().incrementMonitoringCount();
  } else {
    ctx.getAudioEngine().decrementMonitoringCount();
  }

  ctx.getAudioEngine().rebuildMonitoredChannelMask();

  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "track.monitoringChanged";
  broadcast["trackId"] = trackId;
  broadcast["monitoring"] = monitoring;
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

SetTrackMuteCommand::SetTrackMuteCommand(std::string trackId, bool mute)
    : trackId(std::move(trackId)), mute(mute) {}

void SetTrackMuteCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* track = song->getTracksManager()->findTrackById(trackId);
  if (!track) return;

  track->setMute(mute);

  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "track.muteChanged";
  broadcast["trackId"] = trackId;
  broadcast["mute"] = mute;
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

SetTrackSoloCommand::SetTrackSoloCommand(std::string trackId, bool solo)
    : trackId(std::move(trackId)), solo(solo) {}

void SetTrackSoloCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* track = song->getTracksManager()->findTrackById(trackId);
  if (!track) return;

  track->setSolo(solo);

  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "track.soloChanged";
  broadcast["trackId"] = trackId;
  broadcast["solo"] = solo;
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

SetTrackVolumeCommand::SetTrackVolumeCommand(std::string trackId, float volume)
    : trackId(std::move(trackId)), volume(volume) {}

void SetTrackVolumeCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* track = song->getTracksManager()->findTrackById(trackId);
  if (!track) return;

  track->setVolume(volume);

  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "track.volumeChanged";
  broadcast["trackId"] = trackId;
  broadcast["volume"] = track->volume;
  ctx.getWebSocketServer().broadcast(broadcast.dump());
}

// Auto-registration
REGISTER_COMMAND("audio.listDevices", ListDevicesCommand);

REGISTER_COMMAND_WITH_CREATOR(
    "audio.setDevice", SetAudioDevice,
    [](const nlohmann::json& payload) -> CommandPtr {
      return std::make_unique<SetAudioDeviceCommand>(
          payload.value("deviceType", ""),
          payload.value("outputDevice", ""),
          payload.value("inputDevice", ""),
          payload.value("sampleRate", 0.0),
          payload.value("bufferSize", 0));
    });

REGISTER_COMMAND("audio.listInputs", ListInputsCommand);

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "track.setInput", SetTrackInput,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      if (trackId.empty()) return nullptr;
      int inputChannel = payload.value("inputChannel", -1);
      bool stereo = payload.value("stereo", false);
      return std::make_unique<SetTrackInputCommand>(trackId, inputChannel,
                                                    stereo);
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "track.setMonitoring", SetTrackMonitoring,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      if (trackId.empty()) return nullptr;
      bool monitoring = payload.value("monitoring", false);
      return std::make_unique<SetTrackMonitoringCommand>(trackId, monitoring);
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "track.setMute", SetTrackMute,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      if (trackId.empty()) return nullptr;
      bool mute = payload.value("mute", false);
      return std::make_unique<SetTrackMuteCommand>(trackId, mute);
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "track.setSolo", SetTrackSolo,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      if (trackId.empty()) return nullptr;
      bool solo = payload.value("solo", false);
      return std::make_unique<SetTrackSoloCommand>(trackId, solo);
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "track.setVolume", SetTrackVolume,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      if (trackId.empty()) return nullptr;
      float volume = payload.value("volume", 0.4f);
      return std::make_unique<SetTrackVolumeCommand>(trackId, volume);
    });
