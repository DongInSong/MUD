#pragma once

#include "world/room.hpp"
#include <memory>
#include <string>

namespace mud {
class session; // Forward declaration

class Player {
public:
  Player(const std::string &name);

  const std::string &get_name() const;
  void send_message(const std::string &message);
  void set_session(std::weak_ptr<session> session);

  void set_location(std::shared_ptr<world::Room> room, int x, int y);
  std::shared_ptr<world::Room> get_room() const;
  int get_x() const;
  int get_y() const;

private:
  std::string name_;
  std::weak_ptr<session> session_;
  std::shared_ptr<world::Room> current_room_;
  int x_ = 0;
  int y_ = 0;
};

} // namespace mud
