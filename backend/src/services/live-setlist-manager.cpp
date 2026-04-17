// backend/src/services/live-setlist-manager.cpp
#include "services/live-setlist-manager.hpp"

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "events/event-engine.hpp"
#include "model/song.hpp"
#include "services/project-manager.hpp"
#include "services/song-preloader.hpp"
#include "services/songs-manager.hpp"
#include "websocket/broadcast-helpers.hpp"

void LiveSetlistManager::load(const Setlist& setlist,
                              const std::vector<Song*>& allSongs,
                              AppContext& ctx) {
  setlistSnapshot = setlist;
  songIds.clear();
  for (const auto& entry : setlist.getEntries())
    songIds.push_back(entry.songId);

  active = true;
  currentIndex = 0;

  // Load first song — no events, setlist.loaded is broadcast below
  changeSong(0, ctx, false);

  // Fire event engine triggers
  ctx.getEventEngine().fire("setlist.started", ctx);

  broadcast::send(ctx.getWebSocketServer(), "setlist.loaded",
                  {{"setlist", setlistSnapshot.toJson()},
                   {"currentIndex", currentIndex},
                   {"song", currentSongJson(ctx)}});
}

void LiveSetlistManager::loadSingle(Song* song, AppContext& ctx) {
  // Create implicit in-memory setlist
  Setlist s = Setlist::create(song->getName());
  SetlistEntry entry;
  entry.songId = song->getId();
  entry.transition = SetlistTransition::Stop;
  s.setEntries({entry});

  setlistSnapshot = s;
  songIds = {song->getId()};
  active = true;
  currentIndex = 0;

  ctx.getAudioEngine().stop();
  ctx.getAudioEngine().loadSong(song);
  ctx.getLoopManager().reset();
  ctx.getLoopManager().setLoops(song->getLoops());
  ctx.getEventEngine().loadRules(ctx.getSongsManager().getProjectEventRules(),
                                 setlistSnapshot.getEventRules(),
                                 song->getEventRules());
  ctx.getEventEngine().fire("song.loaded", ctx);

  broadcast::send(ctx.getWebSocketServer(), "setlist.loaded",
                  {{"setlist", setlistSnapshot.toJson()},
                   {"currentIndex", 0},
                   {"song", song->toJson()}});
}

void LiveSetlistManager::unload(AppContext& ctx) {
  if (!active) return;
  active = false;
  ctx.getSongPreloader().onLiveModeExited();
  broadcast::send(ctx.getWebSocketServer(), "setlist.unloaded", {});
}

bool LiveSetlistManager::advance(AppContext& ctx) {
  if (!active || currentIndex + 1 >= static_cast<int>(songIds.size()))
    return false;
  changeSong(currentIndex + 1, ctx);
  return true;
}

bool LiveSetlistManager::previous(AppContext& ctx) {
  if (!active || currentIndex <= 0) return false;
  changeSong(currentIndex - 1, ctx);
  return true;
}

bool LiveSetlistManager::goTo(int index, AppContext& ctx) {
  if (!active || index < 0 || index >= static_cast<int>(songIds.size()))
    return false;
  changeSong(index, ctx);
  return true;
}

void LiveSetlistManager::onSongEndReached(AppContext& ctx) {
  if (!active) return;
  const auto& entries = setlistSnapshot.getEntries();
  if (currentIndex >= static_cast<int>(entries.size())) return;

  auto transition = entries[currentIndex].transition;

  if (transition == SetlistTransition::Continue) {
    if (currentIndex + 1 < static_cast<int>(songIds.size())) {
      changeSong(currentIndex + 1, ctx);
    } else {
      // Last song: fire ended event
      ctx.getEventEngine().fire("setlist.ended", ctx);
      broadcast::send(ctx.getWebSocketServer(), "setlist.ended",
                      {{"setlist", setlistSnapshot.toJson()}});
    }
  } else {
    // Stop transition: halt playback at the end position
    ctx.getAudioEngine().pause();

    if (currentIndex + 1 >= static_cast<int>(songIds.size())) {
      ctx.getEventEngine().fire("setlist.ended", ctx);
      broadcast::send(ctx.getWebSocketServer(), "setlist.ended",
                      {{"setlist", setlistSnapshot.toJson()}});
    }
  }
}

void LiveSetlistManager::changeSong(int newIndex, AppContext& ctx, bool fireEvents) {
  // Save previous current song before updating index
  Song* oldSong = getSongAt(currentIndex, ctx);

  currentIndex = newIndex;
  auto& sm = ctx.getSongsManager();
  Song* song = sm.getSongById(songIds[newIndex]);
  if (!song) return;

  // If song not loaded (not preloaded), load now — may have brief gap
  if (song->getLoadState() != SongLoadState::Loaded) {
    song->loadAudio(ctx.getProjectManager().getAudioDir());
  }

  ctx.getAudioEngine().loadSong(song);
  ctx.getLoopManager().reset();
  ctx.getLoopManager().setLoops(song->getLoops());
  ctx.getEventEngine().loadRules(sm.getProjectEventRules(),
                                 setlistSnapshot.getEventRules(),
                                 song->getEventRules());
  ctx.getEventEngine().loadMarkers(song->getMarkers());
  ctx.getEventEngine().fire("song.loaded", ctx);

  // Preload next
  std::vector<Song*> orderedSongs;
  for (const auto& id : songIds) {
    if (auto* s = sm.getSongById(id)) orderedSongs.push_back(s);
  }
  ctx.getSongPreloader().onSongChanged(song, orderedSongs,
                                       ctx.getProjectManager().getAudioDir());

  // Unload the previous current song if it is no longer needed.
  // Skip if it became the newly preloaded next song (e.g. navigating backward).
  if (oldSong && oldSong != song &&
      oldSong != ctx.getSongPreloader().getPreloadedSong() &&
      oldSong->getLoadState() == SongLoadState::Loaded) {
    juce::Logger::writeToLog("[LiveSetlistManager] Unloading previous song: " +
                             juce::String(oldSong->getName()));
    oldSong->unloadAudio();
  }

  if (fireEvents) {
    ctx.getEventEngine().fire("setlist.songChanged", ctx);
    broadcast::send(ctx.getWebSocketServer(), "setlist.songChanged",
                    {{"currentIndex", currentIndex},
                     {"song", song->toJson()}});
  }
}

Song* LiveSetlistManager::getSongAt(int index, AppContext& ctx) const {
  if (index < 0 || index >= static_cast<int>(songIds.size())) return nullptr;
  return ctx.getSongsManager().getSongById(songIds[index]);
}

nlohmann::json LiveSetlistManager::currentSongJson(AppContext& ctx) const {
  auto* s = getSongAt(currentIndex, ctx);
  return s ? s->toJson() : nlohmann::json(nullptr);
}
