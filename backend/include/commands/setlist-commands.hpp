// backend/include/commands/setlist-commands.hpp
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "commands/command.hpp"
#include "events/event-rule.hpp"
#include "model/setlist.hpp"

class ListSetlistsCommand : public Command {
 public:
  void execute(AppContext& ctx) override;
};

class CreateSetlistCommand : public Command {
 public:
  CreateSetlistCommand(std::string name, std::vector<SetlistEntry> entries,
                       std::vector<EventRule> events);
  void execute(AppContext& ctx) override;

 private:
  std::string name;
  std::vector<SetlistEntry> entries;
  std::vector<EventRule> events;
};

class UpdateSetlistCommand : public Command {
 public:
  UpdateSetlistCommand(std::string id, std::optional<std::string> name,
                       std::optional<std::vector<SetlistEntry>> entries,
                       std::optional<std::vector<EventRule>> events);
  void execute(AppContext& ctx) override;

 private:
  std::string id;
  std::optional<std::string> name;
  std::optional<std::vector<SetlistEntry>> entries;
  std::optional<std::vector<EventRule>> events;
};

class DeleteSetlistCommand : public Command {
 public:
  explicit DeleteSetlistCommand(std::string id);
  void execute(AppContext& ctx) override;

 private:
  std::string id;
};
