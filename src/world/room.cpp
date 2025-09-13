#include "world/room.hpp"
#include <utility>

namespace mud {
namespace world {

Room::Room(const std::string &id, const std::string &name,
           const std::string &description, int width, int height)
    : id_(id), name_(name), description_(description), width_(width),
      height_(height) {
  tiles_.resize(height, std::vector<Tile>(width));
}

const std::string &Room::get_id() const { return id_; }

const std::string &Room::get_name() const { return name_; }

const std::string &Room::get_description() const { return description_; }

int Room::get_width() const { return width_; }

int Room::get_height() const { return height_; }

const Tile &Room::get_tile(int x, int y) const {
  if (y >= 0 && y < height_ && x >= 0 && x < width_) {
    return tiles_[y][x];
  }
  // Return a default or error tile, or throw an exception
  static const Tile empty_tile;
  return empty_tile;
}

void Room::link(const std::string &direction, std::shared_ptr<Room> room) {
  exits_[direction] = std::move(room);
}

std::shared_ptr<Room> Room::get_exit(const std::string &direction) const {
  auto it = exits_.find(direction);
  if (it != exits_.end()) {
    return it->second;
  }
  return nullptr;
}

void Room::add_object(int x, int y, const Object &object) {
  if (x >= 0 && x < width_ && y >= 0 && y < height_) {
    tiles_[y][x].objects.push_back(object);
  }
}

void Room::add_portal(const Portal &portal) {
  if (portal.x >= 0 && portal.x < width_ && portal.y >= 0 &&
      portal.y < height_) {
    tiles_[portal.y][portal.x].portal = std::make_shared<Portal>(portal);
  }
}

} // namespace world
} // namespace mud
