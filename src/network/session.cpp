#include "network/session.hpp"
#include "network/server.hpp"
#include "nlp/natural_language_processor.hpp"
#include "players/player.hpp"
#include "world/room.hpp"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include "utils/text_format.hpp"
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
        s->deliver(utils::color::event(obj.name + "와(과) 상호작용할 수 있습니다."));
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
  deliver(utils::color::color(utils::color::SYSTEM, "환영합니다! 이름을 입력해주세요:"));
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
    deliver(utils::color::color(utils::color::ERROR_, "이미 사용중인 이름입니다. 다른 이름을 입력해주세요:"));
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

  std::string title = "MUD에 오신 것을 환영합니다!";
  std::string description = "안녕하세요, " + player_->get_name() + "님!\n이곳은 상상과 모험이 가득한 세계입니다. '/d말'을 입력하여 가능한 명령어들을 확인해보세요.";
  deliver(utils::text_format::create_boxed_message(title, description, {}));

  std::string join_msg = utils::color::join(player_->get_name() + "님이 게임에 참여했습니다.");
  utils::Logger::instance().log(player_->get_name() + "님이 게임에 참여했습니다.");
  server_.broadcast(join_msg, shared_from_this());

  command_handler_.handle("LOOK", {});
}

void session::handle_message(const std::string &msg) {
  if (msg.empty()) {
    return;
  }

  if (mode_ == PlayerMode::MAP) {
    if (msg == "__UP__") {
        command_handler_.handle("NORTH", {});
    } else if (msg == "__DOWN__") {
        command_handler_.handle("SOUTH", {});
    } else if (msg == "__LEFT__") {
        command_handler_.handle("WEST", {});
    } else if (msg == "__RIGHT__") {
        command_handler_.handle("EAST", {});
    } else if (msg[0] == '/') {
        process_command(msg.substr(1));
    }
    return;
  }

  //if command is not say, shout, whisper, clear or player is not chat mode -> self deliver
  if (msg[0] != '/' && !isInChatMode_) {
      deliver(player_->get_name() + ": " + msg);
  }

  if (msg[0] == '/') {
    process_command(msg.substr(1));
  } else {
    if (isInChatMode_) {
        command_handler_.handle("SAY", {msg});
        return;
    }

    auto self(shared_from_this());
    boost::asio::post(server_.get_thread_pool(), [self, msg]() {
        ParsedCommand parsed = self->server_.get_nlp().parse(msg);
        
        boost::asio::post(self->socket_.get_executor(), [self, parsed, msg]() {
            self->handle_parsed_command(parsed, msg);
        });
    });
  }
}

void session::handle_parsed_command(const ParsedCommand& parsed, const std::string& original_msg) {
    if (parsed.command.empty()) {
        if (!parsed.args.empty()) {
            deliver(utils::color::info(parsed.args[0]));
        } else {
            utils::Logger::instance().log(player_->get_name() + "님의 입력을 명령으로 인식하지 못했습니다: " + original_msg);
            deliver(utils::color::error("명령을 인식하지 못했습니다. 채팅 모드로 전환하려면 /chat, /ㅊ 명령을 사용하세요."));
        }
    } else {
        utils::Logger::instance().log(player_->get_name() + "님의 입력을 명령으로 인식했습니다: " + original_msg);
        std::string canonical_command = server_.get_command_manager().get_canonical_command(parsed.command);

        if (!canonical_command.empty()) {
            command_handler_.handle(canonical_command, parsed.args);
        } else {
            command_handler_.handle(parsed.command, parsed.args);
        }
    }
}

void session::process_command(const std::string &input) {
  std::istringstream iss(input);
  std::string alias;
  iss >> alias;

  std::string command =
      server_.get_command_manager().get_canonical_command(alias);

  if (command.empty()) {
    deliver(utils::color::error("알 수 없는 명령어입니다: " + alias));
    return;
  }

  // std::vector<std::string> args;
  // std::string arg;
  // if (std::getline(iss, arg)) {
  //   if (!arg.empty() && arg[0] == ' ') {
  //     arg = arg.substr(1);
  //   }
  //   if (!arg.empty()) {
  //     args.push_back(arg);
  //   }
  // }

std::vector<std::string> args;
std::string arg;
while (iss >> arg) {
    args.push_back(arg);
}

  command_handler_.handle(command, args);
}

void session::toggle_chat_mode() {
    isInChatMode_ = !isInChatMode_;
    if (isInChatMode_) {
        deliver(utils::color::system("채팅 모드로 전환합니다. 모든 입력은 채팅으로 처리됩니다."));
    } else {
        deliver(utils::color::system("명령 모드로 전환합니다."));
    }
}

void session::set_mode(PlayerMode mode) {
    mode_ = mode;
}

PlayerMode session::get_mode() const {
    return mode_;
}

} // namespace mud
