#pragma once

#include <optional>
#include <string>
#include "commands/command.hpp"
#include "model/marker.hpp"

/** List all markers for the active song. Edit-mode only. */
class MarkerListCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

/** Add a new marker to the active song. Edit-mode only. */
class MarkerAddCommand : public Command {
 public:
  MarkerAddCommand(Marker marker);
  void execute(AppContext& ctx) override;
 private:
  Marker marker;
};

/** Remove a marker from the active song. Edit-mode only. */
class MarkerRemoveCommand : public Command {
 public:
  MarkerRemoveCommand(std::string markerId);
  void execute(AppContext& ctx) override;
 private:
  std::string markerId;
};

/** Update name and/or position of an existing marker. Edit-mode only. */
class MarkerUpdateCommand : public Command {
 public:
  MarkerUpdateCommand(std::string markerId,
                      std::optional<std::string> name,
                      std::optional<double> position);
  void execute(AppContext& ctx) override;
 private:
  std::string markerId;
  std::optional<std::string> name;
  std::optional<double> position;
};
