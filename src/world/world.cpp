#include "world/world.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

namespace mud {
namespace world {

using json = nlohmann::json;

World::World(const std::string &maps_directory, const std::string &items_file, const std::string &npcs_file) {
  item_manager_.load_items(items_file);
  npc_manager_.load_npcs(npcs_file);
  load_world_from_files(maps_directory);
}

void World::add_room(const std::string &id, std::shared_ptr<Room> room) {
  rooms_[id] = std::move(room);
}

std::shared_ptr<Room> World::get_room(const std::string &id) const {
  auto it = rooms_.find(id);
  if (it != rooms_.end()) {
    return it->second;
  }
  return nullptr;
}

const ItemManager& World::get_item_manager() const {
    return item_manager_;
}

const NpcManager& World::get_npc_manager() const {
    return npc_manager_;
}

void World::load_world_from_files(const std::string &maps_directory) {
  std::map<std::string, json> map_data;

  // 1. Read all json files and create Room objects
  for (const auto &entry :
       std::filesystem::directory_iterator(maps_directory)) {
    if (entry.path().extension() == ".json") {
      std::ifstream f(entry.path());
      json data = json::parse(f);

      std::string id = data["id"];
      map_data[id] = data;

      auto room = std::make_shared<Room>(
          id, data["name"], data["description"],
          data["size"]["width"], data["size"]["height"]);

    //   for (const auto &obj_data : data["objects"]) {
    //     Object obj{obj_data["type"], obj_data["name"],
    //                obj_data["description"], obj_data["is_interactable"]};
    //     room->add_object(obj_data["x"], obj_data["y"], obj);
    //   }
    for (const auto &obj_data : data["objects"]) {
Object obj{
    obj_data["type"].get<std::string>(),
    obj_data["name"].get<std::string>(),
    obj_data["is_interactable"].get<bool>(),
    obj_data["description"].get<std::string>(),
    obj_data.contains("item_id") ? obj_data["item_id"].get<std::string>() : "",
    obj_data.contains("npc_id") ? obj_data["npc_id"].get<int>() : 0
};
  room->add_object(obj_data["x"], obj_data["y"], obj);
}


      for (const auto &portal_data : data["portals"]) {
        Portal portal{portal_data["x"].get<int>(),
                      portal_data["y"].get<int>(),
                      portal_data["target_map"].get<std::string>(),
                      portal_data["target_x"].get<int>(),
                      portal_data["target_y"].get<int>(),
                      portal_data["description"].get<std::string>()};
        room->add_portal(portal);
      }
      add_room(id, room);
    }
  }

  // 2. Link rooms using exit data
  for (const auto &pair : map_data) {
    const std::string &current_room_id = pair.first;
    const json &data = pair.second;
    auto current_room = get_room(current_room_id);

    if (current_room && data.contains("exits")) {
      for (auto it = data["exits"].begin(); it != data["exits"].end(); ++it) {
        std::string direction = it.key();
        std::string target_room_id = it.value();
        auto target_room = get_room(target_room_id);
        if (target_room) {
          current_room->link(direction, target_room);
        }
      }
    }
  }
}

} // namespace world
} // namespace mud
