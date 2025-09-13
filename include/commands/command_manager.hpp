#pragma once

#include <map>
#include <string>
#include <vector>

namespace mud {

class CommandManager {
public:
  CommandManager(const std::string &command_file_path);

  std::string get_canonical_command(const std::string &alias) const;

private:
  void load_commands(const std::string &command_file_path);

  std::map<std::string, std::string> alias_to_command_;
};

} // namespace mud
