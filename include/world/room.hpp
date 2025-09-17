#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace mud {
namespace world {

struct Portal {
  int x;
  int y;
  std::string target_map;
  int target_x;
  int target_y;
  std::string description;
};

struct Object {
  std::string type;
  std::string name;
  bool is_interactable;
  std::string description;
  std::string item_id;
  int npc_id = 0;
};

struct Tile {
  std::vector<Object> objects;
  std::optional<Portal> portal;
};

class Room {
public:
  Room(const std::string &id, const std::string &name,
       const std::string &description, int width, int height);

  const std::string &get_id() const;
  const std::string &get_name() const;
  const std::string &get_description() const;
  int get_width() const;
  int get_height() const;

  void add_object(int x, int y, const Object &object);
  void remove_object(int x, int y, const std::string& object_name);
  void add_portal(const Portal &portal);
  const Tile &get_tile(int x, int y) const;

  void link(const std::string &direction, std::shared_ptr<Room> room);
  std::shared_ptr<Room> get_exit(const std::string &direction) const;

private:
  std::string id_;
  std::string name_;
  std::string description_;
  int width_;
  int height_;
  std::vector<std::vector<Tile>> tiles_;
  std::map<std::string, std::weak_ptr<Room>> exits_;
};

} // namespace world
} // namespace mud
