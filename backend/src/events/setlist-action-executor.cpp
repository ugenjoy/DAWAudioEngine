#include <juce_core/juce_core.h>

#include "app-context.hpp"
#include "audio/audio-engine-core.hpp"
#include "events/action-executor.hpp"
#include "events/event-engine.hpp"
#include "events/setlist-action-executor.hpp"
#include "model/setlist.hpp"
#include "services/session.hpp"
#include "services/song-preloader.hpp"
#include "websocket/broadcast-helpers.hpp"

// ── Helper: change to a new performing index with audio loading ───────────────

static void performingChangeSong(int newIndex, AppContext& ctx) {
  auto& session = ctx.getSession();
  auto* project = session.getProject();
  if (!project || !session.isLive()) return;

  auto setlistId = session.getPerformingSetlistId();
  if (!setlistId) return;

  const Setlist* setlist = project->setlists().findById(*setlistId);
  if (!setlist) return;

  const auto& entries = setlist->getEntries();
  if (newIndex < 0 || newIndex >= static_cast<int>(entries.size())) return;

  int currentIdx = session.getPerformingIndex();
  Song* oldSong = project->songs().getSongById(entries[currentIdx].songId);

  if (!session.setPerformingIndex(newIndex)) return;

  Song* newSong = project->songs().getSongById(entries[newIndex].songId);
  if (!newSong) return;

  ctx.getAudioEngine().stop();

  if (oldSong && oldSong != newSong &&
      oldSong != ctx.getSongPreloader().getPreloadedSong() &&
      oldSong->getLoadState() == SongLoadState::Loaded) {
    oldSong->unloadAudio();
  }

  const std::string audioDir = project->getAudioDir();
  if (newSong->getLoadState() != SongLoadState::Loaded) {
    newSong->loadAudio(audioDir);
  }

  ctx.getAudioEngine().loadSong(newSong);
  ctx.getLoopManager().reset();
  ctx.getLoopManager().setLoops(newSong->getLoops());
  ctx.getEventEngine().loadRules(
      project->songs().getProjectEventRules(),
      setlist->getEventRules(),
      newSong->getEventRules());
  ctx.getEventEngine().loadMarkers(newSong->getMarkers());
  ctx.getEventEngine().fire("song.loaded", ctx);

  // Preload next
  std::vector<Song*> orderedSongs;
  for (const auto& e : entries) {
    if (auto* s = project->songs().getSongById(e.songId))
      orderedSongs.push_back(s);
  }
  ctx.getSongPreloader().onSongChanged(newSong, orderedSongs, audioDir);

  ctx.getEventEngine().fire("setlist.songChanged", ctx);
  broadcast::send(ctx.getWebSocketServer(), "setlist.songChanged",
                  {{"currentIndex", newIndex}, {"song", newSong->toJson()}});
}

class SetlistNextExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "setlist.next"; }
  void execute(const EventAction&, AppContext& ctx) override {
    auto& session = ctx.getSession();
    if (!session.isLive()) return;
    int next = session.getPerformingIndex() + 1;
    auto* project = session.getProject();
    auto setlistId = session.getPerformingSetlistId();
    if (!project || !setlistId) return;
    const Setlist* setlist = project->setlists().findById(*setlistId);
    if (!setlist) return;
    bool ok = next < static_cast<int>(setlist->getEntries().size());
    if (ok) performingChangeSong(next, ctx);
    juce::Logger::writeToLog(
        juce::String("[SetlistNextExecutor] advance: ") + (ok ? "OK" : "no-op"));
  }
};

class SetlistPrevExecutor : public ActionExecutor {
 public:
  std::string getActionType() const override { return "setlist.prev"; }
  void execute(const EventAction&, AppContext& ctx) override {
    auto& session = ctx.getSession();
    if (!session.isLive()) return;
    int prev = session.getPerformingIndex() - 1;
    bool ok = prev >= 0;
    if (ok) performingChangeSong(prev, ctx);
    juce::Logger::writeToLog(
        juce::String("[SetlistPrevExecutor] previous: ") + (ok ? "OK" : "no-op"));
  }
};

std::vector<std::unique_ptr<ActionExecutor>> createSetlistActionExecutors() {
  std::vector<std::unique_ptr<ActionExecutor>> v;
  v.push_back(std::make_unique<SetlistNextExecutor>());
  v.push_back(std::make_unique<SetlistPrevExecutor>());
  return v;
}
