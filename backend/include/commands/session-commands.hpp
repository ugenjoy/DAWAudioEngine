#pragma once

#include <string>

#include "commands/command.hpp"

class OpenProjectCommand : public Command {
 public:
  explicit OpenProjectCommand(std::string path);
  void execute(AppContext& ctx) override;

 private:
  std::string path;
};

class CloseProjectCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class StartEditingCommand : public Command {
 public:
  explicit StartEditingCommand(std::string songId);
  void execute(AppContext& ctx) override;

 private:
  std::string songId;
};

class StartPerformingCommand : public Command {
 public:
  explicit StartPerformingCommand(std::string setlistId);
  void execute(AppContext& ctx) override;

 private:
  std::string setlistId;
};

class StopActivityCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class SessionAdvanceCommand : public Command {
 public:
  /** @param keepPlaying  When true, do not stop the audio engine before
   *  loading the next song.  Used by Continue transitions for gapless
   *  playback (loadSong resets endPositionFired and playhead, audio resumes
   *  on the new song without a transport stop).  Defaults to false for
   *  manual advances, which behave like a normal stop+load. */
  explicit SessionAdvanceCommand(bool keepPlaying = false);
  void execute(AppContext& ctx) override;

 private:
  bool keepPlaying;
};

class SessionPreviousCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class SessionGoToCommand : public Command {
 public:
  explicit SessionGoToCommand(int index);
  void execute(AppContext& ctx) override;

 private:
  int index;
};

class GetSessionCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};
