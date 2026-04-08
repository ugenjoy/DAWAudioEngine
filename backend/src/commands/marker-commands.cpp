#include "commands/marker-commands.hpp"

#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "events/event-engine.hpp"
#include "model/song.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── Helpers ────────────────────────────────────────────────────────────────

static void broadcastMarkerList(AppContext& ctx) {
  Song* song = ctx.getAudioEngine().getActiveSong();
  nlohmann::json markersJson = nlohmann::json::array();
  if (song) {
    for (const auto& m : song->getMarkers()) {
      markersJson.push_back(m.toJson());
    }
  }
  broadcast::send(ctx.getWebSocketServer(), "marker.listUpdated",
                  {{"markers", markersJson}});
}

static void reloadEngineMarkers(AppContext& ctx) {
  Song* song = ctx.getAudioEngine().getActiveSong();
  if (song) {
    ctx.getEventEngine().loadMarkers(song->getMarkers());
  }
}

// ── MarkerListCommand ──────────────────────────────────────────────────────

void MarkerListCommand::execute(AppContext& ctx) {
  broadcastMarkerList(ctx);
  juce::Logger::writeToLog("[MarkerListCommand] Listed markers");
}

// ── MarkerAddCommand ───────────────────────────────────────────────────────

MarkerAddCommand::MarkerAddCommand(Marker marker) : marker(std::move(marker)) {}

void MarkerAddCommand::execute(AppContext& ctx) {
  Song* song = ctx.getAudioEngine().getActiveSong();
  if (!song) {
    juce::Logger::writeToLog("[MarkerAddCommand] No active song");
    return;
  }
  song->addMarker(marker);
  reloadEngineMarkers(ctx);
  broadcastMarkerList(ctx);
  juce::Logger::writeToLog("[MarkerAddCommand] Added marker '" +
                           juce::String(marker.id) + "'");
}

// ── MarkerRemoveCommand ────────────────────────────────────────────────────

MarkerRemoveCommand::MarkerRemoveCommand(std::string markerId)
    : markerId(std::move(markerId)) {}

void MarkerRemoveCommand::execute(AppContext& ctx) {
  Song* song = ctx.getAudioEngine().getActiveSong();
  bool removed = song && song->removeMarker(markerId);
  reloadEngineMarkers(ctx);
  broadcastMarkerList(ctx);
  juce::Logger::writeToLog("[MarkerRemoveCommand] Remove '" +
                           juce::String(markerId) + "': " +
                           (removed ? "OK" : "not found"));
}

// ── MarkerUpdateCommand ────────────────────────────────────────────────────

MarkerUpdateCommand::MarkerUpdateCommand(Marker updated)
    : updated(std::move(updated)) {}

void MarkerUpdateCommand::execute(AppContext& ctx) {
  Song* song = ctx.getAudioEngine().getActiveSong();
  bool ok = song && song->updateMarker(updated.id, updated);
  reloadEngineMarkers(ctx);
  broadcastMarkerList(ctx);
  juce::Logger::writeToLog("[MarkerUpdateCommand] Update '" +
                           juce::String(updated.id) + "': " +
                           (ok ? "OK" : "not found"));
}

// ── Auto-registration ──────────────────────────────────────────────────────

REGISTER_EDIT_COMMAND("marker.list", MarkerListCommand);

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "marker.add", MarkerAdd,
    [](const nlohmann::json& payload) -> CommandPtr {
      if (!payload.contains("name")) return nullptr;
      Marker m;
      m.id       = juce::Uuid().toDashedString().toStdString();
      m.name     = payload["name"].get<std::string>();
      m.position = payload.value("position", 0.0);
      return std::make_unique<MarkerAddCommand>(std::move(m));
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "marker.remove", MarkerRemove,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string id = payload.value("markerId", "");
      if (id.empty()) return nullptr;
      return std::make_unique<MarkerRemoveCommand>(std::move(id));
    });

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "marker.update", MarkerUpdate,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string id = payload.value("markerId", "");
      if (id.empty()) return nullptr;
      Marker m;
      m.id       = id;
      m.name     = payload.value("name",     std::string(""));
      m.position = payload.value("position", 0.0);
      return std::make_unique<MarkerUpdateCommand>(std::move(m));
    });
