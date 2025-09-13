#include "network/session.hpp"
#include "network/server.hpp"
#include "players/player.hpp"
#include "world/room.hpp"
#include "utils/color.hpp"
#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mud {

void look_at_tile(session *s) {
  auto player = s->get_player();
  if (!player || !player->get_room())
    return;

  auto room = player->get_room();
  const auto &tile = room->get_tile(player->get_x(), player->get_y());

  for (const auto &obj : tile.objects) {
    s->deliver(utils::color::event(obj.description));
    if(obj.is_interactable) {
        s->deliver(utils::color::event("You can interact with " + obj.name));
        // Add interaction logic here
    }
  }
  if (tile.portal) {
    s->deliver(utils::color::portal(tile.portal->description));
  }
}

session::session(tcp::socket socket, server &server)
    : socket_(std::move(socket)), server_(server), command_handler_(*this) {
  try {
    remote_endpoint_str_ = socket_.remote_endpoint().address().to_string() +
                           ":" +
                           std::to_string(socket_.remote_endpoint().port());
  } catch (const boost::system::system_error &e) {
    std::cerr << "Failed to get remote endpoint: " << e.what() << std::endl;
    remote_endpoint_str_ = "unknown";
  }
}

void session::start() {
  server_.join(shared_from_this());
  deliver(utils::color::color(utils::color::SYSTEM, "Welcome! Please enter your name:"));
  do_read();
}

void session::stop() {
  bool expected = false;
  if (closing_.compare_exchange_strong(expected, true)) {
    server_.leave(shared_from_this());
    socket_.close();
  }
}

void session::deliver(const std::string &msg) {
  bool write_in_progress = !write_msgs_.empty();
  write_msgs_.push_back(msg + "\n");
  if (!write_in_progress) {
    do_write();
  }
}

std::shared_ptr<Player> session::get_player() const { return player_; }
server &session::get_server() { return server_; }
bool session::is_logged_in() const { return is_logged_in_; }

void session::do_read() {
  auto self(shared_from_this());
  boost::asio::async_read_until(
      socket_, buffer_, '\n',
      [self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
          std::istream is(&self->buffer_);
          std::string msg;
          std::getline(is, msg);

          if (!msg.empty() && msg.back() == '\r') {
            msg.pop_back();
          }

          if (!self->is_logged_in_) {
            self->handle_initial_input(msg);
          } else {
            self->handle_message(msg);
          }
          self->do_read();
        } else if (ec != boost::asio::error::eof &&
                   ec != boost::asio::error::connection_reset) {
          std::cerr << "Read error from " << self->remote_endpoint_str_
                    << ": " << ec.message() << std::endl;
          self->stop();
        } else {
          self->stop();
        }
      });
}

void session::do_write() {
  auto self(shared_from_this());
  boost::asio::async_write(
      socket_, boost::asio::buffer(write_msgs_.front()),
      [self](boost::system::error_code ec, std::size_t /*length*/) {
        if (!ec) {
          self->write_msgs_.pop_front();
          if (!self->write_msgs_.empty()) {
            self->do_write();
          }
        } else if (ec != boost::asio::error::eof &&
                   ec != boost::asio::error::connection_reset) {
          std::cerr << "Write error to " << self->remote_endpoint_str_
                    << ": " << ec.message() << std::endl;
          self->stop();
        } else {
          self->stop();
        }
      });
}

void session::handle_initial_input(const std::string &input) {
  player_ = server_.add_player(input);
  if (!player_) {
    deliver(utils::color::color(utils::color::ERROR_, "Name is already taken. Please choose another name:"));
    return;
  }
  player_->set_session(shared_from_this());

  auto starting_room = server_.get_world().get_room("town_square");
  if (starting_room) {
    player_->set_location(starting_room, starting_room->get_width() / 2,
                          starting_room->get_height() / 2);
  }

  is_logged_in_ = true;
  deliver("\033[2J\033[H"); // Clear screen
//   deliver("\033[1;32mHello, " + player_->get_name() +
//                                "! Welcome to the MUD.\033[0m");
deliver(utils::color::color(utils::color::SAY ,"Hello, " + player_->get_name() +
                               "! Welcome to the MUD."));   

  std::string join_msg = utils::color::join(player_->get_name() + " has joined the game.");
  server_.broadcast(join_msg, shared_from_this());

  process_command("look");
}

void session::handle_message(const std::string &msg) {
  if (msg.empty()) {
    return;
  }

  if (msg[0] == '/') {
    process_command(msg.substr(1));
  } else {
    process_command("say " + msg);
  }
}

void session::process_command(const std::string &input) {
  std::istringstream iss(input);
  std::string alias;
  iss >> alias;

  std::string command =
      server_.get_command_manager().get_canonical_command(alias);

  if (command.empty()) {
    // deliver(utils::color::ERROR("Unknown command: " + alias));
    deliver(utils::color::system("Unknown command: " + alias));
    return;
  }

  std::vector<std::string> args;
  std::string arg;
  if (std::getline(iss, arg)) {
    if (!arg.empty() && arg[0] == ' ') {
      arg = arg.substr(1);
    }
    if (!arg.empty()) {
      args.push_back(arg);
    }
  }

// std::vector<std::string> args;
// std::string arg;
// while (iss >> arg) {
//     args.push_back(arg);
// }

  command_handler_.handle(command, args);
}

} // namespace mud
