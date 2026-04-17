#include "commands/setlist-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "commands/command-factory.hpp"
#include "events/event-rule.hpp"
#include "services/project-manager.hpp"
#include "services/setlist-manager.hpp"
#include "services/songs-manager.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── ListSetlistsCommand ──────────────────────────────────────────────────────

void ListSetlistsCommand::execute(AppContext& ctx) {
  broadcast::send(ctx.getWebSocketServer(), "setlist.listUpdated",
                  {{"setlists", ctx.getSetlistManager().toJson()}});
}

REGISTER_COMMAND(
    "setlist.list", ListSetlistsCommand);

// ── CreateSetlistCommand ─────────────────────────────────────────────────────

CreateSetlistCommand::CreateSetlistCommand(std::string name,
                                           std::vector<SetlistEntry> entries,
                                           std::vector<EventRule> events)
    : name(std::move(name)),
      entries(std::move(entries)),
      events(std::move(events)) {}

void CreateSetlistCommand::execute(AppContext& ctx) {
  auto& setlistManager = ctx.getSetlistManager();
  setlistManager.create(name, entries, events);

  ctx.getProjectManager().saveProject(
      ctx.getProjectManager().getCurrentProjectPath(),
      ctx.getSongsManager(), &setlistManager);

  broadcast::send(ctx.getWebSocketServer(), "setlist.listUpdated",
                  {{"setlists", setlistManager.toJson()}});
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "setlist.create", CreateSetlist,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string name = payload.value("name", "");
      if (name.empty()) return nullptr;

      std::vector<SetlistEntry> entries;
      if (payload.contains("entries") && payload["entries"].is_array()) {
        for (const auto& e : payload["entries"])
          entries.push_back(SetlistEntry::fromJson(e));
      }

      std::vector<EventRule> events;
      if (payload.contains("events") && payload["events"].is_array()) {
        for (const auto& e : payload["events"])
          events.push_back(EventRule::fromJson(e));
      }

      return std::make_unique<CreateSetlistCommand>(name, entries, events);
    });

// ── UpdateSetlistCommand ─────────────────────────────────────────────────────

UpdateSetlistCommand::UpdateSetlistCommand(
    std::string id, std::optional<std::string> name,
    std::optional<std::vector<SetlistEntry>> entries,
    std::optional<std::vector<EventRule>> events)
    : id(std::move(id)),
      name(std::move(name)),
      entries(std::move(entries)),
      events(std::move(events)) {}

void UpdateSetlistCommand::execute(AppContext& ctx) {
  auto& setlistManager = ctx.getSetlistManager();
  if (!setlistManager.update(id, name, entries, events)) return;

  ctx.getProjectManager().saveProject(
      ctx.getProjectManager().getCurrentProjectPath(),
      ctx.getSongsManager(), &setlistManager);

  broadcast::send(ctx.getWebSocketServer(), "setlist.listUpdated",
                  {{"setlists", setlistManager.toJson()}});
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "setlist.update", UpdateSetlist,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string id = payload.value("setlistId", "");
      if (id.empty()) return nullptr;

      std::optional<std::string> name;
      if (payload.contains("name") && payload["name"].is_string())
        name = payload["name"].get<std::string>();

      std::optional<std::vector<SetlistEntry>> entries;
      if (payload.contains("entries") && payload["entries"].is_array()) {
        std::vector<SetlistEntry> e;
        for (const auto& entry : payload["entries"])
          e.push_back(SetlistEntry::fromJson(entry));
        entries = std::move(e);
      }

      std::optional<std::vector<EventRule>> events;
      if (payload.contains("events") && payload["events"].is_array()) {
        std::vector<EventRule> ev;
        for (const auto& event : payload["events"])
          ev.push_back(EventRule::fromJson(event));
        events = std::move(ev);
      }

      return std::make_unique<UpdateSetlistCommand>(id, name, entries, events);
    });

// ── DeleteSetlistCommand ─────────────────────────────────────────────────────

DeleteSetlistCommand::DeleteSetlistCommand(std::string id)
    : id(std::move(id)) {}

void DeleteSetlistCommand::execute(AppContext& ctx) {
  auto& setlistManager = ctx.getSetlistManager();
  if (!setlistManager.remove(id)) return;

  ctx.getProjectManager().saveProject(
      ctx.getProjectManager().getCurrentProjectPath(),
      ctx.getSongsManager(), &setlistManager);

  broadcast::send(ctx.getWebSocketServer(), "setlist.listUpdated",
                  {{"setlists", setlistManager.toJson()}});
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "setlist.delete", DeleteSetlist,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string id = payload.value("setlistId", "");
      if (id.empty()) return nullptr;
      return std::make_unique<DeleteSetlistCommand>(id);
    });

// ── LoadSetlistCommand ───────────────────────────────────────────────────────

LoadSetlistCommand::LoadSetlistCommand(std::string setlistId)
    : setlistId(std::move(setlistId)) {}

void LoadSetlistCommand::execute(AppContext& ctx) {
  auto* setlist = ctx.getSetlistManager().findById(setlistId);
  if (!setlist) return;

  auto& songsManager = ctx.getSongsManager();
  std::vector<Song*> songs;
  for (const auto& entry : setlist->getEntries()) {
    Song* song = songsManager.getSongById(entry.songId);
    if (song) songs.push_back(song);
  }

  ctx.getLiveSetlistManager().load(*setlist, songs, ctx);
}

REGISTER_COMMAND_WITH_CREATOR(
    "setlist.load", LoadSetlist,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string id = payload.value("setlistId", "");
      if (id.empty()) return nullptr;
      return std::make_unique<LoadSetlistCommand>(id);
    });

// ── LoadSingleSongLiveCommand ────────────────────────────────────────────────

LoadSingleSongLiveCommand::LoadSingleSongLiveCommand(std::string songId)
    : songId(std::move(songId)) {}

void LoadSingleSongLiveCommand::execute(AppContext& ctx) {
  Song* song = ctx.getSongsManager().getSongById(songId);
  if (!song) return;

  ctx.getLiveSetlistManager().loadSingle(song, ctx);
}

REGISTER_COMMAND_WITH_CREATOR(
    "setlist.loadSingle", LoadSingleSongLive,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string id = payload.value("songId", "");
      if (id.empty()) return nullptr;
      return std::make_unique<LoadSingleSongLiveCommand>(id);
    });

// ── UnloadSetlistCommand ─────────────────────────────────────────────────────

void UnloadSetlistCommand::execute(AppContext& ctx) {
  ctx.getLiveSetlistManager().unload(ctx);
}

REGISTER_COMMAND(
    "setlist.unload", UnloadSetlistCommand);

// ── AdvanceSetlistCommand ────────────────────────────────────────────────────

void AdvanceSetlistCommand::execute(AppContext& ctx) {
  ctx.getLiveSetlistManager().advance(ctx);
}

REGISTER_COMMAND(
    "setlist.advance", AdvanceSetlistCommand);

// ── PreviousSetlistCommand ───────────────────────────────────────────────────

void PreviousSetlistCommand::execute(AppContext& ctx) {
  ctx.getLiveSetlistManager().previous(ctx);
}

REGISTER_COMMAND(
    "setlist.previous", PreviousSetlistCommand);

// ── GoToSetlistCommand ───────────────────────────────────────────────────────

GoToSetlistCommand::GoToSetlistCommand(int index) : index(index) {}

void GoToSetlistCommand::execute(AppContext& ctx) {
  ctx.getLiveSetlistManager().goTo(index, ctx);
}

REGISTER_COMMAND_WITH_CREATOR(
    "setlist.goTo", GoToSetlist,
    [](const nlohmann::json& payload) -> CommandPtr {
      if (!payload.contains("index")) return nullptr;
      int index = payload.value("index", 0);
      return std::make_unique<GoToSetlistCommand>(index);
    });
