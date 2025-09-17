#include "players/player.hpp"
#include "network/session.hpp"
#include <utility>

namespace mud {

Player::Player(const std::string &name) : name_(name) {}

const std::string &Player::get_name() const { return name_; }

void Player::send_message(const std::string &message) {
  if (auto spt = session_.lock()) {
    spt->deliver(message);
  }
}

void Player::set_session(std::weak_ptr<session> session) {
  session_ = std::move(session);
}

void Player::set_location(std::shared_ptr<world::Room> room, int x, int y) {
  current_room_ = std::move(room);
  x_ = x;
  y_ = y;
}

std::shared_ptr<world::Room> Player::get_room() const { return current_room_; }

int Player::get_x() const { return x_; }

int Player::get_y() const { return y_; }

int Player::get_sight_radius() const { return sight_radius_; }

} // namespace mud
