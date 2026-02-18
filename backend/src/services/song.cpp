#include "model/song.hpp"

#include "audio/audio-context.hpp"

Song::Song()
    : id(juce::Uuid().toDashedString().toStdString()),
      tempo(120.0f),
      tracksManager(std::make_unique<TracksManager>()) {}

Song::~Song() = default;

void Song::addTrack(std::unique_ptr<AudioTrack> track) {
  tracksManager->addTrack(std::move(track));
}

void Song::removeTrack() {
  // To implement
}

void Song::render(juce::AudioBuffer<float>& mixBuffer,
                  juce::AudioBuffer<float>& trackBuffer, int numSamples,
                  double pos) {
  // double pos = currentPosition.load(std::memory_order_relaxed);
  tracksManager->renderTracks(mixBuffer, trackBuffer, numSamples, pos, tempo);

  auto const& ctx = AudioContext::getInstance();
}

nlohmann::json Song::toJson() const {
  nlohmann::json j;
  j["id"] = id;
  j["name"] = name;
  j["tempo"] = tempo;
  j["tracks"] = tracksManager->toJson();
  return j;
}

std::unique_ptr<Song> Song::fromJson(const nlohmann::json& j) {
  auto song = std::make_unique<Song>();

  // Restore ID if present
  if (j.contains("id")) {
    song->id = j["id"].get<std::string>();
  }

  song->name = j["name"].get<std::string>();
  song->tempo = j.value("tempo", 120.0f);

  // Load tracks
  if (j.contains("tracks")) {
    song->tracksManager->loadFromJson(j["tracks"]);
  }

  return song;
}