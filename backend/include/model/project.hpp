#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "model/setlist.hpp"
#include "model/song.hpp"

/**
 * Represents a daw project.
 */
struct Project {
  std::string id;  // UUID
  std::string name;
  std::string path;

  std::vector<Song*> songs;
  std::vector<Setlist*> setlists;

  nlohmann::json toJson() const;
  static Project fromJson(const nlohmann::json& j);
};
