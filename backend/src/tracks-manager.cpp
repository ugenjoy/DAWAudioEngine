#include "tracks-manager.hpp"
#include "beat-track.hpp"

TracksManager::TracksManager() = default;
TracksManager::~TracksManager() = default;

void TracksManager::addTrack(std::unique_ptr<AudioTrack> track) {
  tracks.push_back(std::move(track));
}

void TracksManager::removeTrack() {
  // To implement
}

void TracksManager::renderTracks(juce::AudioBuffer<float>& mixBuffer,
                                 juce::AudioBuffer<float>& trackBuffer,
                                 int numSamples,
                                 double currentPosition,
                                 float tempo) {
  for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
    trackBuffer.clear();

    if (!tracks[trackIdx]->mute) {
      tracks[trackIdx]->renderBlock(trackBuffer, 0, numSamples, currentPosition,
                                    tempo);
    }

    for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel) {
      mixBuffer.addFrom(channel, 0, trackBuffer, 0, 0, numSamples);
    }
  }
}

nlohmann::json TracksManager::toJson() const {
  nlohmann::json j = nlohmann::json::array();

  for (const auto& track : tracks) {
    j.push_back(track->toJson());
  }

  return j;
}

void TracksManager::loadFromJson(const nlohmann::json& j) {
  tracks.clear();

  if (!j.is_array()) {
    return;
  }

  for (const auto& trackJson : j) {
    if (!trackJson.contains("type")) {
      continue;
    }

    std::string type = trackJson["type"];

    if (type == "BeatTrack") {
      tracks.push_back(BeatTrack::fromJson(trackJson));
    }
    // Future track types can be added here:
    // else if (type == "AudioFileTrack") {
    //   tracks.push_back(AudioFileTrack::fromJson(trackJson));
    // }
  }
}