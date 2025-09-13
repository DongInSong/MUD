#include "commands/command_manager.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace mud {

using json = nlohmann::json;

CommandManager::CommandManager(const std::string &command_file_path) {
  load_commands(command_file_path);
}

void CommandManager::load_commands(const std::string &command_file_path) {
  std::ifstream f(command_file_path);
  json data = json::parse(f);

  for (const auto &command_data : data["commands"]) {
    std::string canonical_name = command_data["name"];
    for (const auto &alias : command_data["aliases"]) {
      alias_to_command_[alias] = canonical_name;
    }
  }
}

std::string
CommandManager::get_canonical_command(const std::string &alias) const {
  auto it = alias_to_command_.find(alias);
  if (it != alias_to_command_.end()) {
    return it->second;
  }
  return ""; // Not found
}

} // namespace mud
