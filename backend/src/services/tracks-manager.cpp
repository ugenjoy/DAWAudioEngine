#include "model/tracks-manager.hpp"

#include "audio/audio-file-track.hpp"
#include "audio/beat-track.hpp"

#include <cmath>

namespace {
/** @brief Equal-power panning: compute left/right gains from pan in [-1, 1] */
inline std::pair<float, float> computePanGains(float pan) {
  float angle = (pan + 1.0f) * 0.25f *
                juce::MathConstants<float>::pi;  // 0..π/2
  return {std::cos(angle), std::sin(angle)};
}
}  // namespace

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
                                 const juce::AudioBuffer<float>& inputBuffer,
                                 int numSamples, double currentPosition,
                                 float tempo, bool isPlaying) {
  for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
    trackBuffer.clear();

    if (!tracks[trackIdx]->mute) {
      int inCh = tracks[trackIdx]->getInputChannel();
      bool shouldMonitor = tracks[trackIdx]->isMonitoring() && inCh >= 0 &&
                           inCh < inputBuffer.getNumChannels();

      if (shouldMonitor) {
        // Route input audio to track instead of rendering clips
        if (tracks[trackIdx]->isInputStereo()) {
          trackBuffer.copyFrom(0, 0, inputBuffer, inCh, 0, numSamples);
          int rightCh =
              std::min(inCh + 1, inputBuffer.getNumChannels() - 1);
          trackBuffer.copyFrom(1, 0, inputBuffer, rightCh, 0, numSamples);
        } else {
          // Mono: duplicate to both channels
          trackBuffer.copyFrom(0, 0, inputBuffer, inCh, 0, numSamples);
          trackBuffer.copyFrom(1, 0, inputBuffer, inCh, 0, numSamples);
        }
        // Apply track volume
        for (int ch = 0; ch < trackBuffer.getNumChannels(); ++ch) {
          trackBuffer.applyGain(ch, 0, numSamples, tracks[trackIdx]->volume);
        }
      } else if (isPlaying) {
        // Only render clips/synth when transport is running
        tracks[trackIdx]->renderBlock(trackBuffer, 0, numSamples,
                                      currentPosition, tempo);
      }
    }

    // Apply equal-power panning
    if (trackBuffer.getNumChannels() >= 2) {
      auto [gainL, gainR] = computePanGains(tracks[trackIdx]->pan);
      trackBuffer.applyGain(0, 0, numSamples, gainL);
      trackBuffer.applyGain(1, 0, numSamples, gainR);
    }

    for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel) {
      int srcChannel = std::min(channel, trackBuffer.getNumChannels() - 1);
      mixBuffer.addFrom(channel, 0, trackBuffer, srcChannel, 0, numSamples);
    }
  }
}

void TracksManager::sampleRateChanged() {
  for (auto& track : tracks) {
    track->sampleRateChanged();
  }
}

AudioTrack* TracksManager::findTrackById(const std::string& id) const {
  for (const auto& track : tracks) {
    if (track->getId() == id) {
      return track.get();
    }
  }
  return nullptr;
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
    } else if (type == "AudioFileTrack") {
      tracks.push_back(AudioFileTrack::fromJson(trackJson));
    }
  }
}