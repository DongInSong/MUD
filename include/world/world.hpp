#pragma once

#include "world/item_manager.hpp"
#include "world/room.hpp"
#include <map>
#include <memory>
#include <string>

namespace mud {
namespace world {

class World {
public:
  World(const std::string &maps_directory, const std::string &items_file);

  void add_room(const std::string &id, std::shared_ptr<Room> room);
  std::shared_ptr<Room> get_room(const std::string &id) const;
  const ItemManager& get_item_manager() const;

private:
  void load_world_from_files(const std::string &maps_directory);

  std::map<std::string, std::shared_ptr<Room>> rooms_;
  ItemManager item_manager_;
};

} // namespace world
} // namespace mud
