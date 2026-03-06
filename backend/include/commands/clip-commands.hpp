#pragma once

#include <string>

#include "commands/command.hpp"

class AddClipCommand : public Command {
 public:
  AddClipCommand(std::string trackId, std::string fileName, double position,
                 std::string name);
  void execute(AppContext& ctx) override;

 private:
  std::string trackId;
  std::string fileName;
  double position;
  std::string clipName;
};
