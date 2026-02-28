#include "model/tracks-manager.hpp"

#include <cmath>

#include "audio/audio-context.hpp"
#include "audio/audio-file-track.hpp"
#include "audio/metronome-track.hpp"

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

bool TracksManager::removeTrack(const std::string& id) {
  auto it = std::find_if(tracks.begin(), tracks.end(),
                         [&](const auto& t) { return t->getId() == id; });
  if (it == tracks.end()) return false;
  tracks.erase(it);
  return true;
}

bool TracksManager::renameTrack(const std::string& id, const std::string& name) {
  auto* track = findTrackById(id);
  if (!track) return false;
  track->name = name;
  return true;
}

bool TracksManager::reorderTrack(const std::string& id, int newIndex) {
  auto it = std::find_if(tracks.begin(), tracks.end(),
                         [&](const auto& t) { return t->getId() == id; });
  if (it == tracks.end()) return false;

  int idx = std::clamp(newIndex, 0, static_cast<int>(tracks.size()) - 1);
  auto track = std::move(*it);
  tracks.erase(it);
  tracks.insert(tracks.begin() + idx, std::move(track));
  return true;
}

void TracksManager::renderTracks(juce::AudioBuffer<float>& mixBuffer,
                                 juce::AudioBuffer<float>& trackBuffer,
                                 const juce::AudioBuffer<float>& inputBuffer,
                                 int numSamples, double currentPosition,
                                 float tempo, bool isPlaying) {
  // Check if any track is soloed
  bool hasSolo = false;
  for (const auto& track : tracks) {
    if (track->solo) {
      hasSolo = true;
      break;
    }
  }

  for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
    trackBuffer.clear();

    // Skip muted tracks, and when solo is active skip non-soloed tracks
    bool shouldPlay = !tracks[trackIdx]->mute &&
                      (!hasSolo || tracks[trackIdx]->solo);

    if (shouldPlay) {
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
          trackBuffer.applyGain(ch, 0, numSamples, tracks[trackIdx]->getLinearGain());
        }
      } else if (isPlaying) {
        // Only render when transport is running
        if (tracks[trackIdx]->frozen.load(std::memory_order_acquire) &&
            tracks[trackIdx]->frozenPeriodSamples > 0) {
          // Read from frozen buffer with modulo (periodic looping)
          const auto& fb = tracks[trackIdx]->frozenBuffer;
          const int period = tracks[trackIdx]->frozenPeriodSamples;
          const int offset =
              static_cast<int>(currentPosition * AudioContext::getInstance().sampleRate) %
              period;

          for (int ch = 0; ch < trackBuffer.getNumChannels() &&
                           ch < fb.getNumChannels();
               ++ch) {
            int available = period - offset;
            if (available >= numSamples) {
              trackBuffer.copyFrom(ch, 0, fb, ch, offset, numSamples);
            } else {
              // Wrap around: copy tail then head
              trackBuffer.copyFrom(ch, 0, fb, ch, offset, available);
              trackBuffer.copyFrom(ch, available, fb, ch, 0,
                                   numSamples - available);
            }
          }
        } else {
          tracks[trackIdx]->renderBlock(trackBuffer, 0, numSamples,
                                        currentPosition, tempo);
        }
      }
    }

    // Apply equal-power panning
    if (trackBuffer.getNumChannels() >= 2) {
      auto [gainL, gainR] = computePanGains(tracks[trackIdx]->pan);
      trackBuffer.applyGain(0, 0, numSamples, gainL);
      trackBuffer.applyGain(1, 0, numSamples, gainR);
    }

    // Store peak level for VU meter (max absolute value across all channels)
    float peak = shouldPlay ? trackBuffer.getMagnitude(0, numSamples) : 0.0f;
    tracks[trackIdx]->peakLevel.store(peak, std::memory_order_relaxed);

    for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel) {
      int srcChannel = std::min(channel, trackBuffer.getNumChannels() - 1);
      mixBuffer.addFrom(channel, 0, trackBuffer, srcChannel, 0, numSamples);
    }
  }
}

void TracksManager::freezeAll(float tempo, double sampleRate) {
  for (auto& track : tracks) {
    if (track->canFreeze() && !track->isMonitoring()) {
      track->freeze(tempo, sampleRate);
    }
  }
}

void TracksManager::unfreezeAll() {
  for (auto& track : tracks) {
    track->unfreeze();
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

    if (type == "AudioFileTrack") {
      tracks.push_back(AudioFileTrack::fromJson(trackJson));
    }
  }
}