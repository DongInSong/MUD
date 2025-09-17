#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace mud {

class session; // Forward declaration

class CommandHandler {
public:
  CommandHandler(session &s);
  void handle(const std::string &command,
              const std::vector<std::string> &args);

private:
  void setup_commands();
  void look(const std::vector<std::string> &args);
  void move(int dx, int dy, int amount = 1);
  void move_to(const std::vector<std::string> &args);
  void say(const std::vector<std::string> &args);
  void shout(const std::vector<std::string> &args);
  void whisper(const std::vector<std::string> &args);
  void quit(const std::vector<std::string> &args);
  void clear(const std::vector<std::string> &args);
  void interact(const std::vector<std::string> &args);
  void talk(const std::vector<std::string> &args);
  void get(const std::vector<std::string> &args);
  void show_map(const std::vector<std::string> &args);

  session &session_;
  std::map<std::string,
           std::function<void(const std::vector<std::string> &)>>
      commands_;
};

} // namespace mud
