#pragma once

#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "model/project.hpp"

/**
 * Owns the active Project and tracks the current user activity (editing a song
 * or performing a setlist).  Replaces the scattered state that previously lived
 * in ModeManager, LiveSetlistManager, and ProjectManager.
 *
 * A single Listener is notified after every state transition so that callers
 * (e.g. WebSocket command handlers) can broadcast the appropriate events.
 */
class Session {
 public:
  enum class Activity { None, Editing, Performing };

  // ---------------------------------------------------------------------------
  // Project lifecycle
  // ---------------------------------------------------------------------------

  /** Store a new project, stopping any running activity first. */
  void openProject(std::unique_ptr<Project>);

  /** Stop any running activity then release the project. */
  void closeProject();

  Project* getProject();
  const Project* getProject() const;

  // ---------------------------------------------------------------------------
  // Activity transitions (implicit stop of previous activity)
  // ---------------------------------------------------------------------------

  /** Begin editing the given song.  Implicitly stops Performing if active. */
  bool startEditing(const std::string& songId);

  /** Begin performing the given setlist.  Implicitly stops Editing if active. */
  bool startPerforming(const std::string& setlistId);

  /** Return to Activity::None, clearing editing/performing state. */
  void stopActivity();

  // ---------------------------------------------------------------------------
  // Performing navigation
  // ---------------------------------------------------------------------------

  /** Advance to the next entry in the performing setlist. */
  bool advancePerforming();

  /** Go back to the previous entry in the performing setlist. */
  bool previousPerforming();

  /** Jump to a specific index in the performing setlist. */
  bool setPerformingIndex(int index);

  /** Called when the transport reaches the end of the current song.
   *  Handles automatic Continue/Stop/Pause transitions. */
  void onSongEndReached();

  /** Set audio callbacks used by onSongEndReached to trigger stop/pause. */
  void setAudioCallbacks(std::function<void()> stopFn,
                         std::function<void()> pauseFn);

  /** Set the callback used by onSongEndReached to trigger an automatic
   *  advance on a Continue transition.  Implementations typically queue a
   *  SessionAdvanceCommand so the new song is loaded through the same path
   *  as a manual advance (markers reset, audio reload, preload, broadcast). */
  void setAdvanceCallback(std::function<void()> advanceFn);

  // ---------------------------------------------------------------------------
  // Queries
  // ---------------------------------------------------------------------------

  Activity getActivity() const;

  /** Returns true when activity == Editing. */
  bool isEdit() const;

  /** Returns true when activity == Performing. */
  bool isLive() const;

  std::optional<std::string> getEditingSongId() const;
  std::optional<std::string> getPerformingSetlistId() const;
  int getPerformingIndex() const;

  /**
   * Returns the Song that is currently in focus:
   *   - Editing   → the song being edited
   *   - Performing → the song at the current performing index
   *   - None       → nullptr
   */
  Song* getFocusedSong() const;

  // ---------------------------------------------------------------------------
  // Observability
  // ---------------------------------------------------------------------------

  using Listener = std::function<void(const Session&, Activity prev)>;

  /** Replace the current listener (pass nullptr to disable). */
  void setListener(Listener);

  // ---------------------------------------------------------------------------
  // Serialisation — full WS snapshot (excludes playback state)
  // ---------------------------------------------------------------------------

  nlohmann::json toJson() const;

 private:
  std::unique_ptr<Project> project;
  Activity activity = Activity::None;
  std::string editingSongId;
  std::string performingSetlistId;
  int performingIndex = 0;
  Listener listener;
  std::function<void()> stopAudio;
  std::function<void()> pauseAudio;
  std::function<void()> advanceFn;

  /** Fire the listener if one is registered. */
  void fireListener(Activity prev);
};
