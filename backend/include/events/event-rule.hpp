#pragma once

#include <nlohmann/json.hpp>
#include <string>

/**
 * Represents the action part of an event rule.
 * Uses an open params field to support any action type (midi.send, osc.send, etc.)
 */
struct EventAction {
  std::string type;       // e.g. "midi.send"
  nlohmann::json params;  // type-specific params, e.g. { "device": "...", "message": [...] }

  nlohmann::json toJson() const;
  static EventAction fromJson(const nlohmann::json& j);
};

/**
 * A programmable event rule: trigger → action.
 * Stored at project level and/or song level.
 */
struct EventRule {
  std::string id;       // UUID
  std::string trigger;  // e.g. "song.loaded"
  nlohmann::json triggerParams;  // optional MIDI input filter: device, channel, note/cc, threshold
  EventAction action;
  bool enabled = true;

  nlohmann::json toJson() const;
  static EventRule fromJson(const nlohmann::json& j);
};
