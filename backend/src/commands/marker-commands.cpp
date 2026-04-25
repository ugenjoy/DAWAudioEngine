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

REGISTER_EDIT_COMMAND("marker.list", MarkerListCommand);

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

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "marker.add", MarkerAdd,
    [](const nlohmann::json& payload) -> CommandPtr {
      if (!payload.contains("name")) return nullptr;
      Marker m;
      m.id = juce::Uuid().toDashedString().toStdString();
      m.name = payload["name"].get<std::string>();
      m.position = payload.value("position", 0.0);
      return std::make_unique<MarkerAddCommand>(std::move(m));
    });

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

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "marker.remove", MarkerRemove,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string id = payload.value("markerId", "");
      if (id.empty()) return nullptr;
      return std::make_unique<MarkerRemoveCommand>(std::move(id));
    });

// ── MarkerUpdateCommand ────────────────────────────────────────────────────

MarkerUpdateCommand::MarkerUpdateCommand(std::string markerId,
                                         std::optional<std::string> name,
                                         std::optional<double> position)
    : markerId(std::move(markerId)),
      name(std::move(name)),
      position(std::move(position)) {}

void MarkerUpdateCommand::execute(AppContext& ctx) {
  Song* song = ctx.getAudioEngine().getActiveSong();
  bool ok = song && song->updateMarker(markerId, name, position);
  reloadEngineMarkers(ctx);
  broadcastMarkerList(ctx);
  juce::Logger::writeToLog("[MarkerUpdateCommand] Update '" +
                           juce::String(markerId) + "': " +
                           (ok ? "OK" : "not found"));
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "marker.update", MarkerUpdate,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string id = payload.value("markerId", "");
      if (id.empty()) return nullptr;
      std::optional<std::string> name;
      std::optional<double> position;
      if (payload.contains("name")) name = payload["name"].get<std::string>();
      if (payload.contains("position")) position = payload["position"].get<double>();
      return std::make_unique<MarkerUpdateCommand>(std::move(id), std::move(name), position);
    });
