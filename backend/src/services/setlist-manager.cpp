// backend/src/services/setlist-manager.cpp
#include "services/setlist-manager.hpp"

#include <algorithm>

Setlist& SetlistManager::create(const std::string& name,
                                std::vector<SetlistEntry> entries,
                                std::vector<EventRule> events) {
  Setlist s = Setlist::create(name);
  s.setEntries(std::move(entries));
  s.setEventRules(std::move(events));
  setlists.push_back(std::move(s));
  return setlists.back();
}

bool SetlistManager::update(const std::string& id,
                            std::optional<std::string> name,
                            std::optional<std::vector<SetlistEntry>> entries,
                            std::optional<std::vector<EventRule>> events) {
  auto* s = findById(id);
  if (!s) return false;
  if (name) s->setName(*name);
  if (entries) s->setEntries(std::move(*entries));
  if (events) s->setEventRules(std::move(*events));
  return true;
}

bool SetlistManager::remove(const std::string& id) {
  auto it = std::find_if(setlists.begin(), setlists.end(),
                         [&](const Setlist& s) { return s.getId() == id; });
  if (it == setlists.end()) return false;
  setlists.erase(it);
  return true;
}

Setlist* SetlistManager::findById(const std::string& id) {
  for (auto& s : setlists)
    if (s.getId() == id) return &s;
  return nullptr;
}

std::vector<Setlist*> SetlistManager::getList() {
  std::vector<Setlist*> result;
  result.reserve(setlists.size());
  for (auto& s : setlists) result.push_back(&s);
  return result;
}

void SetlistManager::purgeSong(const std::string& songId) {
  for (auto& setlist : setlists) {
    auto entries = setlist.getEntries();
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                 [&](const SetlistEntry& e) {
                                   return e.songId == songId;
                                 }),
                  entries.end());
    setlist.setEntries(std::move(entries));
  }
  // Remove empty setlists
  setlists.erase(std::remove_if(setlists.begin(), setlists.end(),
                                [](const Setlist& s) {
                                  return s.getEntries().empty();
                                }),
                 setlists.end());
}

nlohmann::json SetlistManager::toJson() const {
  nlohmann::json arr = nlohmann::json::array();
  for (const auto& s : setlists) arr.push_back(s.toJson());
  return arr;
}

void SetlistManager::loadFromJson(const nlohmann::json& j) {
  setlists.clear();
  if (!j.is_array()) return;
  for (const auto& item : j)
    setlists.push_back(Setlist::fromJson(item));
}
