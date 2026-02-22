#include "model/song.hpp"

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

void Song::removeTrack() {
  // To implement
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
  j["metronomeMute"] = metronomeTrack->mute;
  j["metronome"] = metronomeTrack->toJson();
  j["tracks"] = tracksManager->toJson();
  return j;
}

std::unique_ptr<Song> Song::fromJson(const nlohmann::json& j) {
  auto song = std::make_unique<Song>();

  if (j.contains("id")) {
    song->id = j["id"].get<std::string>();
  }

  song->name = j["name"].get<std::string>();
  song->tempo = j.value("tempo", 120.0f);

  // Load metronome settings
  if (j.contains("metronome")) {
    song->metronomeTrack = MetronomeTrack::fromJson(j["metronome"]);
  }

  // Load tracks
  if (j.contains("tracks")) {
    song->tracksManager->loadFromJson(j["tracks"]);
  }

  return song;
}
