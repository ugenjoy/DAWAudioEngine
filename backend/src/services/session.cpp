#include "services/session.hpp"

#include "model/setlist.hpp"

// ---------------------------------------------------------------------------
// Project lifecycle
// ---------------------------------------------------------------------------

void Session::openProject(std::unique_ptr<Project> p)
{
 Activity prev = activity;
 stopActivity();
 project = std::move(p);
 fireListener(prev);
}

void Session::closeProject()
{
 Activity prev = activity;
 stopActivity();
 project.reset();
 fireListener(prev);
}

Project *Session::getProject() { return project.get(); }

const Project *Session::getProject() const { return project.get(); }

// ---------------------------------------------------------------------------
// Activity transitions
// ---------------------------------------------------------------------------

bool Session::startEditing(const std::string_view &songId)
{
 Activity prev = activity;
 if (activity == Activity::Performing)
 {
  // Implicit stop — clear performing state without firing the listener yet.
  activity = Activity::None;
  performingSetlistId.clear();
  performingIndex = 0;
 }
 activity = Activity::Editing;
 editingSongId = songId;
 fireListener(prev);
 return true;
}

bool Session::startPerforming(const std::string_view &setlistId)
{
 Activity prev = activity;
 if (activity == Activity::Editing)
 {
  // Implicit stop — clear editing state without firing the listener yet.
  activity = Activity::None;
  editingSongId.clear();
 }
 activity = Activity::Performing;
 performingSetlistId = setlistId;
 performingIndex = 0;
 fireListener(prev);
 return true;
}

void Session::stopActivity()
{
 if (activity == Activity::None)
  return;
 Activity prev = activity;
 activity = Activity::None;
 editingSongId.clear();
 performingSetlistId.clear();
 performingIndex = 0;
 fireListener(prev);
}

// ---------------------------------------------------------------------------
// Performing navigation
// ---------------------------------------------------------------------------

bool Session::advancePerforming()
{
 if (activity != Activity::Performing || !project)
  return false;

 const Setlist *setlist = project->setlists().findById(performingSetlistId);
 if (!setlist)
  return false;

 if (performingIndex + 1 < static_cast<int>(setlist->getEntries().size()))
 {
  Activity prev = activity;
  ++performingIndex;
  fireListener(prev);
  return true;
 }
 return false;
}

bool Session::previousPerforming()
{
 if (activity != Activity::Performing || !project)
  return false;

 if (performingIndex > 0)
 {
  Activity prev = activity;
  --performingIndex;
  fireListener(prev);
  return true;
 }
 return false;
}

bool Session::setPerformingIndex(int index)
{
 if (activity != Activity::Performing || !project)
  return false;

 const Setlist *setlist = project->setlists().findById(performingSetlistId);
 if (!setlist)
  return false;

 if (index < 0 || index >= static_cast<int>(setlist->getEntries().size()))
  return false;

 Activity prev = activity;
 performingIndex = index;
 fireListener(prev);
 return true;
}

void Session::setAudioCallbacks(std::function<void()> stopFn,
                                std::function<void()> pauseFn)
{
 stopAudio = std::move(stopFn);
 pauseAudio = std::move(pauseFn);
}

void Session::setAdvanceCallback(std::function<void()> fn)
{
 advanceFn = std::move(fn);
}

void Session::onSongEndReached()
{
 if (activity != Activity::Performing || !project)
 {
  // Not performing — fall back to a simple stop
  if (stopAudio)
   stopAudio();
  return;
 }

 const Setlist *setlist = project->setlists().findById(performingSetlistId);
 if (!setlist)
  return;

 const auto &entries = setlist->getEntries();
 if (performingIndex < 0 ||
     performingIndex >= static_cast<int>(entries.size()))
  return;

 const auto transition = entries[performingIndex].transition;

 if (transition == SetlistTransition::Continue)
 {
  if (performingIndex + 1 < static_cast<int>(entries.size()))
  {
   // Delegate to the advance callback so the next song is swapped through
   // the same path as a manual advance (markers reset, preload rotation,
   // broadcast). The advance handler is responsible for keeping playback
   // gapless — Continue must not produce an audible stop.
   if (advanceFn)
    advanceFn();
  }
  else
  {
   // Last song with Continue: setlist ended — stop audio and notify
   if (stopAudio)
    stopAudio();
   fireListener(activity);
  }
 }
 else if (transition == SetlistTransition::Pause)
 {
  if (pauseAudio)
   pauseAudio();
  fireListener(activity);
 }
 else
 {
  // SetlistTransition::Stop (default)
  if (stopAudio)
   stopAudio();
  fireListener(activity);
 }
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

Session::Activity Session::getActivity() const { return activity; }

bool Session::isEdit() const { return activity == Activity::Editing; }

bool Session::isLive() const { return activity == Activity::Performing; }

std::optional<std::string> Session::getEditingSongId() const
{
 if (activity == Activity::Editing)
  return editingSongId;
 return std::nullopt;
}

std::optional<std::string> Session::getPerformingSetlistId() const
{
 if (activity == Activity::Performing)
  return performingSetlistId;
 return std::nullopt;
}

int Session::getPerformingIndex() const { return performingIndex; }

Song *Session::getFocusedSong() const
{
 if (!project)
  return nullptr;

 if (activity == Activity::Editing)
 {
  return project->songs().getSongById(editingSongId);
 }

 if (activity == Activity::Performing)
 {
  const Setlist *setlist = project->setlists().findById(performingSetlistId);
  if (!setlist)
   return nullptr;

  const auto &entries = setlist->getEntries();
  if (performingIndex < 0 ||
      performingIndex >= static_cast<int>(entries.size()))
   return nullptr;

  return project->songs().getSongById(entries[performingIndex].songId);
 }

 return nullptr;
}

// ---------------------------------------------------------------------------
// Observability
// ---------------------------------------------------------------------------

void Session::setListener(Listener l) { listener = std::move(l); }

void Session::fireListener(Activity prev)
{
 if (listener)
  listener(*this, prev);
}

// ---------------------------------------------------------------------------
// Serialisation
// ---------------------------------------------------------------------------

nlohmann::json Session::toJson() const
{
 nlohmann::json j;

 j["hasProject"] = (project != nullptr);

 if (project)
 {
  j["project"] = project->toJson();
 }
 else
 {
  j["project"] = nullptr;
 }

 switch (activity)
 {
 case Activity::Editing:
  j["activity"] = "editing";
  break;
 case Activity::Performing:
  j["activity"] = "performing";
  break;
 default:
  j["activity"] = "none";
  break;
 }

 if (activity == Activity::Editing)
 {
  const Song *song = getFocusedSong();
  j["editing"] = {{"songId", editingSongId},
                  {"song", song ? song->toJson() : nullptr}};
 }
 else
 {
  j["editing"] = nullptr;
 }

 if (activity == Activity::Performing)
 {
  const Song *song = getFocusedSong();
  j["performing"] = {{"setlistId", performingSetlistId},
                     {"currentIndex", performingIndex},
                     {"song", song ? song->toJson() : nullptr}};
 }
 else
 {
  j["performing"] = nullptr;
 }

 return j;
}
