#pragma once

#include "world/room.hpp"
#include <map>
#include <memory>
#include <string>

namespace mud {
namespace world {

class World {
public:
  World(const std::string &maps_directory);

  void add_room(const std::string &id, std::shared_ptr<Room> room);
  std::shared_ptr<Room> get_room(const std::string &id) const;

private:
  void load_world_from_files(const std::string &maps_directory);

  std::map<std::string, std::shared_ptr<Room>> rooms_;
};

} // namespace world
} // namespace mud
