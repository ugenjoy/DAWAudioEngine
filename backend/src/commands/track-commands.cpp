#include "commands/track-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "audio/audio-file-track.hpp"
#include "commands/command-factory.hpp"
#include "model/song.hpp"
#include "websocket/broadcast-helpers.hpp"

namespace {
void broadcastTrackList(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;
  broadcast::send(ctx.getWebSocketServer(), "track.listUpdated",
                  {{"tracks", song->getTracksManager()->toJson()}});
}
}  // namespace

// ── ListDevicesCommand ───────────────────────────────────────────────────────

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

// ── SetAudioDeviceCommand ────────────────────────────────────────────────────

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

  nlohmann::json deviceData = {{"current", ctx.getAudioEngine().getCurrentDeviceInfo()}};
  if (result.isNotEmpty()) {
    deviceData["error"] = result.toStdString();
  }
  broadcast::send(ctx.getWebSocketServer(), "audio.deviceChanged", deviceData);

  // Also broadcast updated input list since channels may have changed
  broadcast::send(ctx.getWebSocketServer(), "audio.inputsList",
                  {{"inputs", ctx.getAudioEngine().getAvailableInputs()}});
}

// ── ListInputsCommand ────────────────────────────────────────────────────────

void ListInputsCommand::execute(AppContext& ctx) {
  nlohmann::json response;
  response["type"] = "response";
  response["event"] = "audio.inputsList";
  response["inputs"] = ctx.getAudioEngine().getAvailableInputs();
  reply(response.dump());
}

// ── SetTrackInputCommand ─────────────────────────────────────────────────────

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

  broadcast::send(ctx.getWebSocketServer(), "track.inputChanged",
                  {{"trackId", trackId}, {"inputChannel", inputChannel}, {"stereo", stereo}});
}

// ── SetTrackMonitoringCommand ────────────────────────────────────────────────

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

  broadcast::send(ctx.getWebSocketServer(), "track.monitoringChanged",
                  {{"trackId", trackId}, {"monitoring", monitoring}});
}

// ── SetTrackMuteCommand ──────────────────────────────────────────────────────

SetTrackMuteCommand::SetTrackMuteCommand(std::string trackId, bool mute)
    : trackId(std::move(trackId)), mute(mute) {}

void SetTrackMuteCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* track = song->getTracksManager()->findTrackById(trackId);
  if (!track) return;

  track->setMute(mute);

  broadcast::send(ctx.getWebSocketServer(), "track.muteChanged",
                  {{"trackId", trackId}, {"mute", mute}});
}

// ── SetTrackSoloCommand ──────────────────────────────────────────────────────

SetTrackSoloCommand::SetTrackSoloCommand(std::string trackId, bool solo)
    : trackId(std::move(trackId)), solo(solo) {}

void SetTrackSoloCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* track = song->getTracksManager()->findTrackById(trackId);
  if (!track) return;

  track->setSolo(solo);

  broadcast::send(ctx.getWebSocketServer(), "track.soloChanged",
                  {{"trackId", trackId}, {"solo", solo}});
}

// ── SetTrackVolumeCommand ────────────────────────────────────────────────────

SetTrackVolumeCommand::SetTrackVolumeCommand(std::string trackId, float volume)
    : trackId(std::move(trackId)), volume(volume) {}

void SetTrackVolumeCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* track = song->getTracksManager()->findTrackById(trackId);
  if (!track) return;

  track->setVolume(volume);

  broadcast::send(ctx.getWebSocketServer(), "track.volumeChanged",
                  {{"trackId", trackId}, {"volume", track->volume}});
}

// ── AddTrackCommand ──────────────────────────────────────────────────────────

AddTrackCommand::AddTrackCommand(std::string name)
    : name(std::move(name)) {}

void AddTrackCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto track = std::make_unique<AudioFileTrack>();
  if (name.empty()) {
    track->name = "Track " + std::to_string(song->getTracksManager()->getTracks().size() + 1);
  } else {
    track->name = name;
  }
  song->addTrack(std::move(track));
  broadcastTrackList(ctx);
}

// ── RemoveTrackCommand ───────────────────────────────────────────────────────

RemoveTrackCommand::RemoveTrackCommand(std::string trackId)
    : trackId(std::move(trackId)) {}

void RemoveTrackCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  if (song->removeTrack(trackId)) {
    broadcastTrackList(ctx);
  }
}

// ── RenameTrackCommand ───────────────────────────────────────────────────────

RenameTrackCommand::RenameTrackCommand(std::string trackId, std::string name)
    : trackId(std::move(trackId)), name(std::move(name)) {}

void RenameTrackCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  if (song->getTracksManager()->renameTrack(trackId, name)) {
    broadcastTrackList(ctx);
  }
}

// ── ReorderTrackCommand ──────────────────────────────────────────────────────

ReorderTrackCommand::ReorderTrackCommand(std::string trackId, int index)
    : trackId(std::move(trackId)), index(index) {}

void ReorderTrackCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  if (song->getTracksManager()->reorderTrack(trackId, index)) {
    broadcastTrackList(ctx);
  }
}

// ── SetTrackColorCommand ─────────────────────────────────────────────────────

SetTrackColorCommand::SetTrackColorCommand(std::string trackId, int color)
    : trackId(std::move(trackId)), color(color) {}

void SetTrackColorCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* track = song->getTracksManager()->findTrackById(trackId);
  if (!track) return;

  track->color = std::clamp(color, 1, 8);

  broadcast::send(ctx.getWebSocketServer(), "track.colorChanged",
                  {{"trackId", trackId}, {"color", track->color}});
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

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "track.add", AddTrack,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string name = payload.value("name", "");
      return std::make_unique<AddTrackCommand>(name);
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "track.remove", RemoveTrack,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      if (trackId.empty()) return nullptr;
      return std::make_unique<RemoveTrackCommand>(trackId);
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "track.rename", RenameTrack,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      std::string name = payload.value("name", "");
      if (trackId.empty() || name.empty()) return nullptr;
      return std::make_unique<RenameTrackCommand>(trackId, name);
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "track.reorder", ReorderTrack,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      if (trackId.empty()) return nullptr;
      int index = payload.value("index", 0);
      return std::make_unique<ReorderTrackCommand>(trackId, index);
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "track.setColor", SetTrackColor,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      if (trackId.empty()) return nullptr;
      int color = payload.value("color", 1);
      return std::make_unique<SetTrackColorCommand>(trackId, color);
    });
