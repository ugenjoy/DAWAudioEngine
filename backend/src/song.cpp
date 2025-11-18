#include "song.hpp"
#include "audio-context.hpp"

Song::Song()
    : id(juce::Uuid().toDashedString().toStdString()),
      tempo(120.0f),
      currentPosition(0.0),
      tracksManager(std::make_unique<TracksManager>()) {}

Song::~Song() = default;

void Song::addTrack(std::unique_ptr<AudioTrack> track) {
  tracksManager->addTrack(std::move(track));
}

void Song::removeTrack() {
  // To implement
}

void Song::render(juce::AudioBuffer<float>& mixBuffer,
                  juce::AudioBuffer<float>& trackBuffer,
                  int numSamples) {
  tracksManager->renderTracks(mixBuffer, trackBuffer, numSamples,
                              currentPosition, tempo);

  auto const& ctx = AudioContext::getInstance();
  currentPosition += (double)numSamples / ctx.sampleRate;
}

nlohmann::json Song::toJson() const {
  nlohmann::json j;
  j["id"] = id;
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

  song->tempo = j.value("tempo", 120.0f);
  song->currentPosition = 0.0;  // Always start at beginning when loading

  // Load tracks
  if (j.contains("tracks")) {
    song->tracksManager->loadFromJson(j["tracks"]);
  }

  return song;
}