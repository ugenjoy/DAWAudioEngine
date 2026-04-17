#include "commands/event-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "commands/command-factory.hpp"
#include "events/event-engine.hpp"
#include "services/songs-manager.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── Helpers ────────────────────────────────────────────────────────────────

static void reloadEventEngineRules(AppContext& ctx) {
  auto& songsManager = ctx.getSongsManager();
  auto& audioEngine = ctx.getAudioEngine();
  Song* activeSong = audioEngine.getActiveSong();
  ctx.getEventEngine().loadRules(
      songsManager.getProjectEventRules(), {},
      activeSong ? activeSong->getEventRules() : std::vector<EventRule>{});
}

static void broadcastEventList(AppContext& ctx) {
  auto& songsManager = ctx.getSongsManager();
  auto& audioEngine = ctx.getAudioEngine();

  nlohmann::json projectEventsJson = nlohmann::json::array();
  for (const auto& rule : songsManager.getProjectEventRules()) {
    projectEventsJson.push_back(rule.toJson());
  }

  nlohmann::json songEventsJson = nlohmann::json::array();
  if (Song* activeSong = audioEngine.getActiveSong()) {
    for (const auto& rule : activeSong->getEventRules()) {
      songEventsJson.push_back(rule.toJson());
    }
  }

  broadcast::send(ctx.getWebSocketServer(), "event.listUpdated",
                  {{"projectEvents", projectEventsJson}, {"songEvents", songEventsJson}});
}

// ── EventListCommand ───────────────────────────────────────────────────────

void EventListCommand::execute(AppContext& ctx) {
  broadcastEventList(ctx);
  juce::Logger::writeToLog("[EventListCommand] Listed events");
}

REGISTER_EDIT_COMMAND("event.list", EventListCommand);

// ── EventAddCommand ────────────────────────────────────────────────────────

EventAddCommand::EventAddCommand(std::string scope, EventRule rule)
    : scope(std::move(scope)), rule(std::move(rule)) {}

void EventAddCommand::execute(AppContext& ctx) {
  auto& songsManager = ctx.getSongsManager();
  auto& audioEngine = ctx.getAudioEngine();

  if (scope == "project") {
    songsManager.addProjectEventRule(rule);
  } else if (scope == "song") {
    Song* activeSong = audioEngine.getActiveSong();
    if (!activeSong) {
      juce::Logger::writeToLog(
          "[EventAddCommand] No active song for song-level event");
      return;
    }
    activeSong->addEventRule(rule);
  } else {
    juce::Logger::writeToLog("[EventAddCommand] Unknown scope: " +
                             juce::String(scope));
    return;
  }

  juce::Logger::writeToLog("[EventAddCommand] Added event rule '" +
                           juce::String(rule.id) + "' to " +
                           juce::String(scope));
  reloadEventEngineRules(ctx);
  broadcastEventList(ctx);
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "event.add", EventAdd,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string scope = payload.value("scope", "");
      if (scope.empty() || !payload.contains("trigger") ||
          !payload.contains("eventAction")) {
        return nullptr;
      }

      EventRule rule;
      rule.id = juce::Uuid().toDashedString().toStdString();
      rule.trigger = payload["trigger"].get<std::string>();
      rule.triggerParams = payload.contains("triggerParams")
                               ? payload["triggerParams"]
                               : nlohmann::json::object();
      rule.action = EventAction::fromJson(payload["eventAction"]);
      rule.enabled = payload.value("enabled", true);

      return std::make_unique<EventAddCommand>(scope, std::move(rule));
    });

// ── EventRemoveCommand ─────────────────────────────────────────────────────

EventRemoveCommand::EventRemoveCommand(std::string scope, std::string eventId)
    : scope(std::move(scope)), eventId(std::move(eventId)) {}

void EventRemoveCommand::execute(AppContext& ctx) {
  auto& songsManager = ctx.getSongsManager();
  auto& audioEngine = ctx.getAudioEngine();
  bool removed = false;

  if (scope == "project") {
    removed = songsManager.removeProjectEventRule(eventId);
  } else if (scope == "song") {
    Song* activeSong = audioEngine.getActiveSong();
    if (activeSong) {
      removed = activeSong->removeEventRule(eventId);
    }
  }

  juce::Logger::writeToLog("[EventRemoveCommand] Remove '" +
                           juce::String(eventId) + "': " +
                           (removed ? "OK" : "not found"));
  reloadEventEngineRules(ctx);
  broadcastEventList(ctx);
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "event.remove", EventRemove,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string scope = payload.value("scope", "");
      std::string eventId = payload.value("eventId", "");
      if (scope.empty() || eventId.empty()) return nullptr;
      return std::make_unique<EventRemoveCommand>(scope, eventId);
    });

// ── EventUpdateCommand ─────────────────────────────────────────────────────

EventUpdateCommand::EventUpdateCommand(std::string scope, EventRule updated)
    : scope(std::move(scope)), updated(std::move(updated)) {}

void EventUpdateCommand::execute(AppContext& ctx) {
  auto& songsManager = ctx.getSongsManager();
  auto& audioEngine = ctx.getAudioEngine();
  bool ok = false;

  if (scope == "project") {
    ok = songsManager.updateProjectEventRule(updated.id, updated);
  } else if (scope == "song") {
    Song* activeSong = audioEngine.getActiveSong();
    if (activeSong) {
      ok = activeSong->updateEventRule(updated.id, updated);
    }
  }

  juce::Logger::writeToLog("[EventUpdateCommand] Update '" +
                           juce::String(updated.id) + "': " +
                           (ok ? "OK" : "not found"));
  reloadEventEngineRules(ctx);
  broadcastEventList(ctx);
}

REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "event.update", EventUpdate,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string scope = payload.value("scope", "");
      std::string eventId = payload.value("eventId", "");
      if (scope.empty() || eventId.empty()) return nullptr;

      EventRule updated;
      updated.id = eventId;
      if (payload.contains("trigger")) {
        updated.trigger = payload["trigger"].get<std::string>();
      }
      if (payload.contains("triggerParams")) {
        updated.triggerParams = payload["triggerParams"];
      }
      if (payload.contains("eventAction")) {
        updated.action = EventAction::fromJson(payload["eventAction"]);
      }
      updated.enabled = payload.value("enabled", true);

      return std::make_unique<EventUpdateCommand>(scope, std::move(updated));
    });
