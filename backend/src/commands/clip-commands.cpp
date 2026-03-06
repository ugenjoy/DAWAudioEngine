#include "commands/clip-commands.hpp"

#include <nlohmann/json.hpp>

#include "app-context.hpp"
#include "audio/audio-clip.hpp"
#include "audio/audio-engine-core.hpp"
#include "audio/audio-file-track.hpp"
#include "commands/command-factory.hpp"
#include "model/song.hpp"
#include "services/project-manager.hpp"
#include "websocket/broadcast-helpers.hpp"

namespace {
void broadcastTrackList(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;
  broadcast::send(ctx.getWebSocketServer(), "track.listUpdated",
                  {{"tracks", song->getTracksManager()->toJson()}});
}
}  // namespace

// ── AddClipCommand ──────────────────────────────────────────────────────────

AddClipCommand::AddClipCommand(std::string trackId, std::string fileName,
                               double position, std::string name)
    : trackId(std::move(trackId)),
      fileName(std::move(fileName)),
      position(position),
      clipName(std::move(name)) {}

void AddClipCommand::execute(AppContext& ctx) {
  auto* song = ctx.getAudioEngine().getActiveSong();
  if (!song) return;

  auto* track = song->getTracksManager()->findTrackById(trackId);
  if (!track) return;

  // Only AudioFileTrack supports clips
  auto* fileTrack = dynamic_cast<AudioFileTrack*>(track);
  if (!fileTrack) return;

  // Resolve audio directory from project path
  std::string audioDir = ctx.getProjectManager().getCurrentProjectPath() + "/audio";

  auto clip = std::make_unique<AudioClip>();
  clip->setFileName(fileName);
  clip->setPosition(position);
  if (!clipName.empty()) {
    clip->setName(clipName);
  } else {
    clip->setName(fileName);
  }

  clip->loadAudioFile(audioDir);

  fileTrack->getClipsManager()->addClip(std::move(clip));
  broadcastTrackList(ctx);
}

// Auto-registration
REGISTER_EDIT_COMMAND_WITH_CREATOR(
    "clip.add", AddClip,
    [](const nlohmann::json& payload) -> CommandPtr {
      std::string trackId = payload.value("trackId", "");
      std::string fileName = payload.value("fileName", "");
      if (trackId.empty() || fileName.empty()) return nullptr;
      double position = payload.value("position", 0.0);
      std::string name = payload.value("name", "");
      return std::make_unique<AddClipCommand>(trackId, fileName, position, name);
    });
