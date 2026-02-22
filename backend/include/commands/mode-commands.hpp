#pragma once

#include "commands/command.hpp"

/**
 * Switches the application to Edit mode.
 * Rejected if playback is currently active.
 * On success, broadcasts {"event": "mode.changed", "mode": "edit"}.
 * On failure, replies with an error {"code": "cannot_edit_while_playing"}.
 */
class SetEditModeCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Switches the application to Live mode.
 * Always succeeds. Broadcasts {"event": "mode.changed", "mode": "live"}.
 */
class SetLiveModeCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/**
 * Replies with the current application mode.
 * Reply: {"event": "mode.current", "mode": "live"|"edit"}
 */
class GetModeCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};
