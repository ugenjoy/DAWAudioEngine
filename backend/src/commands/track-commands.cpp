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

  nlohmann::json broadcast;
  broadcast["type"] = "broadcast";
  broadcast["event"] = "track.monitoringChanged";
  broadcast["trackId"] = trackId;
  broadcast["monitoring"] = monitoring;
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

REGISTER_COMMAND_WITH_CREATOR(
    "track.setInput", SetTrackInput,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      if (trackId.empty()) return nullptr;
      int inputChannel = payload.value("inputChannel", -1);
      bool stereo = payload.value("stereo", false);
      return std::make_unique<SetTrackInputCommand>(trackId, inputChannel,
                                                    stereo);
    });

REGISTER_COMMAND_WITH_CREATOR(
    "track.setMonitoring", SetTrackMonitoring,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      if (trackId.empty()) return nullptr;
      bool monitoring = payload.value("monitoring", false);
      return std::make_unique<SetTrackMonitoringCommand>(trackId, monitoring);
    });
