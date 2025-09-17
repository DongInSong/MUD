#pragma once

#include "world/item_manager.hpp"
#include "world/npc_manager.hpp"
#include "world/room.hpp"
#include <map>
#include <memory>
#include <string>

namespace mud::world {

class World {
public:
  World(const std::string &maps_directory, const std::string &items_file, const std::string &npcs_file);

  void add_room(const std::string &id, std::shared_ptr<Room> room);
  std::shared_ptr<Room> get_room(const std::string &id) const;
  const ItemManager& get_item_manager() const;
  const NpcManager& get_npc_manager() const;

private:
  void load_world_from_files(const std::string &maps_directory);

  std::map<std::string, std::shared_ptr<Room>> rooms_;
  ItemManager item_manager_;
  NpcManager npc_manager_;
};

} // namespace mud::world
