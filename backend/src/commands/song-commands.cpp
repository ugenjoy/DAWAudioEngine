#include "commands/song-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "events/event-engine.hpp"
#include "services/project-manager.hpp"
#include "services/setlist-manager.hpp"
#include "services/songs-manager.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── CreateSongCommand ────────────────────────────────────────────────────────

CreateSongCommand::CreateSongCommand(std::string name)
    : name(std::move(name)) {}

void CreateSongCommand::execute(AppContext& ctx) {
  auto& songsManager = ctx.getSongsManager();
  auto& audioEngine = ctx.getAudioEngine();

  auto song = std::make_unique<Song>();
  song->setName(name);
  Song* raw = song.get();
  songsManager.addSong(std::move(song));

  audioEngine.stop();
  audioEngine.loadSong(raw);

  ctx.getEventEngine().loadRules(songsManager.getProjectEventRules(), {},
                                 raw->getEventRules());
  ctx.getEventEngine().fire("song.loaded", ctx);

  broadcast::send(ctx.getWebSocketServer(), "project.songsUpdated",
                  {{"songs", songsManager.toJson()}});
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "song.create", CreateSong,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string name = payload.value("name", "");
      if (name.empty()) return nullptr;
      return std::make_unique<CreateSongCommand>(name);
    });

// ── SetTempoCommand ─────────────────────────────────────────────────────────

SetTempoCommand::SetTempoCommand(float tempo) : tempo(tempo) {}

void SetTempoCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  song->setTempo(tempo);

  broadcast::send(ctx.getWebSocketServer(), "song.tempoChanged",
                  {{"tempo", tempo}});
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "song.setTempo", SetTempo,
    [](const nlohmann::json& payload) -> CommandPtr {
      float tempo = payload.value("tempo", 0.0f);
      if (tempo < 20.0f || tempo > 999.0f) return nullptr;
      return std::make_unique<SetTempoCommand>(tempo);
    });

// ── SetMetronomeMuteCommand ──────────────────────────────────────────────────

SetMetronomeMuteCommand::SetMetronomeMuteCommand(bool mute) : mute(mute) {}

void SetMetronomeMuteCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* metronome = song->getMetronomeTrack();
  if (!metronome) return;

  metronome->setMute(mute);

  broadcast::send(ctx.getWebSocketServer(), "song.metronomeMuteChanged",
                  {{"mute", mute}});
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "song.setMetronomeMute", SetMetronomeMute,
    [](const nlohmann::json& payload) -> CommandPtr {
      bool mute = payload.value("mute", true);
      return std::make_unique<SetMetronomeMuteCommand>(mute);
    });

// ── RenameSongCommand ───────────────────────────────────────────────────────

RenameSongCommand::RenameSongCommand(std::string uuid, std::string name)
    : uuid(std::move(uuid)), name(std::move(name)) {}

void RenameSongCommand::execute(AppContext& ctx) {
  auto& songsManager = ctx.getSongsManager();
  if (!songsManager.renameSong(uuid, name)) return;

  // If the renamed song is the active song, broadcast song update too
  auto* activeSong = ctx.getAudioEngine().getActiveSong();
  if (activeSong && activeSong->getId() == uuid) {
    broadcast::send(ctx.getWebSocketServer(), "song.loaded",
                    {{"song", activeSong->toJson()}});
  }

  broadcast::send(ctx.getWebSocketServer(), "project.songsUpdated",
                  {{"songs", songsManager.toJson()}});
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "song.rename", RenameSong,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string uuid = payload.value("uuid", "");
      std::string name = payload.value("name", "");
      if (uuid.empty() || name.empty()) return nullptr;
      return std::make_unique<RenameSongCommand>(uuid, name);
    });

// ── ReorderSongCommand ─────────────────────────────────────────────────────

ReorderSongCommand::ReorderSongCommand(std::string uuid, int index)
    : uuid(std::move(uuid)), index(index) {}

void ReorderSongCommand::execute(AppContext& ctx) {
  auto& songsManager = ctx.getSongsManager();
  if (!songsManager.reorderSong(uuid, index)) return;

  broadcast::send(ctx.getWebSocketServer(), "project.songsUpdated",
                  {{"songs", songsManager.toJson()}});
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "song.reorder", ReorderSong,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string uuid = payload.value("uuid", "");
      if (uuid.empty() || !payload.contains("index")) return nullptr;
      int index = payload.value("index", 0);
      return std::make_unique<ReorderSongCommand>(uuid, index);
    });

// ── SetEndPositionCommand ─────────────────────────────────────────────────

SetEndPositionCommand::SetEndPositionCommand(std::string uuid,
                                             std::optional<double> pos)
    : uuid(std::move(uuid)), pos(pos) {}

void SetEndPositionCommand::execute(AppContext& ctx) {
  auto& sm = ctx.getSongsManager();
  if (!sm.setEndPosition(uuid, pos)) return;

  ctx.getProjectManager().saveProject(
      ctx.getProjectManager().getCurrentProjectPath(), sm, nullptr);

  broadcast::send(ctx.getWebSocketServer(), "song.endPositionUpdated",
                  {{"songId", uuid},
                   {"endPosition", pos.has_value()
                                       ? nlohmann::json(pos.value())
                                       : nlohmann::json(nullptr)}});
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "song.setEndPosition", SetEndPosition,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string uuid = payload.value("songId", "");
      if (uuid.empty()) return nullptr;
      std::optional<double> pos;
      if (payload.contains("endPosition") && !payload["endPosition"].is_null())
        pos = payload["endPosition"].get<double>();
      return std::make_unique<SetEndPositionCommand>(uuid, pos);
    });

// ── DeleteSongCommand ─────────────────────────────────────────────────────

DeleteSongCommand::DeleteSongCommand(std::string uuid)
    : uuid(std::move(uuid)) {}

void DeleteSongCommand::execute(AppContext& ctx) {
  auto& sm = ctx.getSongsManager();
  auto* active = ctx.getAudioEngine().getActiveSong();
  // Don't delete the active song
  if (active && active->getId() == uuid) return;
  if (!sm.removeSong(uuid)) return;

  // Purge deleted song from all setlists and persist
  auto& setlistManager = ctx.getSetlistManager();
  setlistManager.purgeSong(uuid);
  ctx.getProjectManager().saveProject(
      ctx.getProjectManager().getCurrentProjectPath(), sm, &setlistManager);

  broadcast::send(ctx.getWebSocketServer(), "song.deleted",
                  {{"songId", uuid}});
  broadcast::send(ctx.getWebSocketServer(), "project.songsUpdated",
                  {{"songs", sm.toJson()}});
  broadcast::send(ctx.getWebSocketServer(), "setlist.listUpdated",
                  {{"setlists", setlistManager.toJson()}});
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "song.delete", DeleteSong,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string uuid = payload.value("songId", "");
      if (uuid.empty()) return nullptr;
      return std::make_unique<DeleteSongCommand>(uuid);
    });
