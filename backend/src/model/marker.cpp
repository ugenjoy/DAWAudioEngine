#include "model/marker.hpp"
#include <juce_core/juce_core.h>

nlohmann::json Marker::toJson() const {
  return {{"id", id}, {"name", name}, {"position", position}};
}

Marker Marker::fromJson(const nlohmann::json& j) {
  Marker m;
  m.id       = j.value("id",       "");
  m.name     = j.value("name",     "");
  m.position = j.value("position", 0.0);
  return m;
}
