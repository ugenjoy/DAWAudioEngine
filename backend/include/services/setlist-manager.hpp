// backend/include/services/setlist-manager.hpp
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "model/setlist.hpp"

/**
 * Manages the collection of setlists for the current project.
 * Owns the setlist objects in memory and provides CRUD operations.
 */
class SetlistManager {
 public:
  SetlistManager() = default;

  Setlist& create(const std::string& name,
                  std::vector<SetlistEntry> entries,
                  std::vector<EventRule> events = {});

  bool update(const std::string& id, std::optional<std::string> name,
              std::optional<std::vector<SetlistEntry>> entries,
              std::optional<std::vector<EventRule>> events);

  bool remove(const std::string& id);

  Setlist* findById(const std::string& id);
  const std::vector<Setlist>& getAll() const { return setlists; }

  /** Remove all entries that reference a deleted songId. */
  void purgeSong(const std::string& songId);

  void clear() { setlists.clear(); }

  nlohmann::json toJson() const;
  void loadFromJson(const nlohmann::json& j);

 private:
  std::vector<Setlist> setlists;
};
