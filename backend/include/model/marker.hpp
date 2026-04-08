#pragma once

#include <nlohmann/json.hpp>
#include <string>

/**
 * A named position on the song timeline.
 * Used for structure visualization and as reference for position triggers and seek actions.
 */
struct Marker {
  std::string id;       // UUID
  std::string name;     // e.g. "Verse 1", "Chorus"
  double position = 0.0; // seconds

  nlohmann::json toJson() const;
  static Marker fromJson(const nlohmann::json& j);
};
