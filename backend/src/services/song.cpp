#include "model/song.hpp"

#include <algorithm>
#include <thread>

#include "audio/audio-context.hpp"

Song::Song()
    : id(juce::Uuid().toDashedString().toStdString()),
      tempo(120.0f),
      tracksManager(std::make_unique<TracksManager>()),
      metronomeTrack(std::make_unique<MetronomeTrack>()) {
  metronomeTrack->mute = true;
}

Song::~Song() = default;

void Song::addTrack(std::unique_ptr<AudioTrack> track) {
  tracksManager->addTrack(std::move(track));
}

bool Song::removeTrack(const std::string& trackId) {
  return tracksManager->removeTrack(trackId);
}

void Song::render(juce::AudioBuffer<float>& mixBuffer,
                  juce::AudioBuffer<float>& trackBuffer,
                  const juce::AudioBuffer<float>& inputBuffer, int numSamples,
                  double pos, bool isPlaying) {
  tracksManager->renderTracks(mixBuffer, trackBuffer, inputBuffer, numSamples,
                              pos, tempo, isPlaying);

  // Render metronome on top of the mix
  if (isPlaying && metronomeTrack && !metronomeTrack->mute) {
    trackBuffer.clear();
    metronomeTrack->renderBlock(trackBuffer, 0, numSamples, pos, tempo);
    for (int ch = 0; ch < mixBuffer.getNumChannels(); ++ch) {
      int srcCh = std::min(ch, trackBuffer.getNumChannels() - 1);
      mixBuffer.addFrom(ch, 0, trackBuffer, srcCh, 0, numSamples);
    }
  }
}

void Song::sampleRateChanged() {
  tracksManager->sampleRateChanged();
}

void Song::freezeAllTracks(float tempo, double sampleRate) {
  tracksManager->freezeAll(tempo, sampleRate);
}

void Song::unfreezeAllTracks() {
  tracksManager->unfreezeAll();
}

nlohmann::json Song::toJson() const {
  nlohmann::json j;
  j["id"] = id;
  j["name"] = name;
  j["tempo"] = tempo;
  if (endPosition.has_value()) {
    j["endPosition"] = endPosition.value();
  }
  j["metronomeMute"] = metronomeTrack->mute;
  j["metronome"] = metronomeTrack->toJson();
  j["tracks"] = tracksManager->toJson();

  nlohmann::json eventsJson = nlohmann::json::array();
  for (const auto& rule : eventRules) {
    eventsJson.push_back(rule.toJson());
  }
  j["events"] = eventsJson;

  nlohmann::json loopsJson = nlohmann::json::array();
  for (const auto& loop : loops) {
    loopsJson.push_back({{"id", loop.id}, {"start", loop.start}, {"end", loop.end}});
  }
  j["loops"] = loopsJson;

  nlohmann::json markersJson = nlohmann::json::array();
  for (const auto& m : markers) {
    markersJson.push_back(m.toJson());
  }
  j["markers"] = markersJson;

  return j;
}

void Song::loadAudio(const std::string& audioDir) {
  SongLoadState expected = SongLoadState::MetadataOnly;
  if (!loadState.compare_exchange_strong(expected, SongLoadState::Loading)) {
    // Already Loading or Loaded
    while (loadState.load() == SongLoadState::Loading) {
      std::this_thread::yield();
    }
    return;
  }
  tracksManager->loadAudio(audioDir);
  loadState.store(SongLoadState::Loaded);
}

void Song::unloadAudio() {
  tracksManager->unloadAudio();
  loadState.store(SongLoadState::MetadataOnly);
}

std::unique_ptr<Song> Song::fromJson(const nlohmann::json& j,
                                      const std::string& audioDir,
                                      bool loadAudio) {
  auto song = std::make_unique<Song>();

  if (j.contains("id")) {
    song->id = j["id"].get<std::string>();
  }

  song->name = j["name"].get<std::string>();
  song->tempo = j.value("tempo", 120.0f);

  if (j.contains("endPosition") && !j["endPosition"].is_null()) {
    song->endPosition = j["endPosition"].get<double>();
  }

  // Load metronome settings
  if (j.contains("metronome")) {
    song->metronomeTrack = MetronomeTrack::fromJson(j["metronome"]);
  }

  // Load tracks
  if (j.contains("tracks")) {
    song->tracksManager->loadFromJson(j["tracks"], audioDir, loadAudio);
  }

  // Load song-level event rules
  if (j.contains("events") && j["events"].is_array()) {
    for (const auto& ruleJson : j["events"]) {
      song->eventRules.push_back(EventRule::fromJson(ruleJson));
    }
  }

  if (j.contains("loops") && j["loops"].is_array()) {
    for (const auto& lj : j["loops"]) {
      Loop loop;
      loop.id = lj.value("id", juce::Uuid().toDashedString().toStdString());
      loop.start = lj.value("start", 0.0);
      loop.end = lj.value("end", 0.0);
      song->loops.push_back(loop);
    }
  }

  if (j.contains("markers") && j["markers"].is_array()) {
    for (const auto& mj : j["markers"]) {
      song->markers.push_back(Marker::fromJson(mj));
    }
  }

  if (loadAudio) {
    song->loadState.store(SongLoadState::Loaded);
  }

  return song;
}

bool Song::removeEventRule(const std::string& ruleId) {
  auto it = std::find_if(eventRules.begin(), eventRules.end(),
                         [&](const EventRule& r) { return r.id == ruleId; });
  if (it == eventRules.end()) return false;
  eventRules.erase(it);
  return true;
}

bool Song::updateEventRule(const std::string& ruleId,
                           const EventRule& updated) {
  auto it = std::find_if(eventRules.begin(), eventRules.end(),
                         [&](const EventRule& r) { return r.id == ruleId; });
  if (it == eventRules.end()) return false;
  *it = updated;
  return true;
}

Loop Song::addLoop(double start, double end) {
  Loop loop;
  loop.id = juce::Uuid().toDashedString().toStdString();
  loop.start = start;
  loop.end = end;
  loops.push_back(loop);
  return loop;
}

bool Song::removeLoop(const std::string& loopId) {
  auto it = std::find_if(loops.begin(), loops.end(),
                         [&](const Loop& l) { return l.id == loopId; });
  if (it == loops.end()) return false;
  loops.erase(it);
  return true;
}

bool Song::updateLoop(const std::string& loopId, double start, double end) {
  auto it = std::find_if(loops.begin(), loops.end(),
                         [&](const Loop& l) { return l.id == loopId; });
  if (it == loops.end()) return false;
  it->start = start;
  it->end = end;
  return true;
}

bool Song::removeMarker(const std::string& markerId) {
  auto it = std::find_if(markers.begin(), markers.end(),
                         [&](const Marker& m) { return m.id == markerId; });
  if (it == markers.end()) return false;
  markers.erase(it);
  return true;
}

bool Song::updateMarker(const std::string& markerId, const Marker& updated) {
  auto it = std::find_if(markers.begin(), markers.end(),
                         [&](const Marker& m) { return m.id == markerId; });
  if (it == markers.end()) return false;
  it->name     = updated.name;
  it->position = updated.position;
  return true;
}
