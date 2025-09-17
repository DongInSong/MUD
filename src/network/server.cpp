#include "network/server.hpp"
#include "network/session.hpp"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <utility>

namespace mud {
server::server(boost::asio::io_context &io_context, const tcp::endpoint &endpoint,
               const std::string &data_path)
    : thread_pool_(4), io_context_(io_context), acceptor_(io_context, endpoint),
      world_(data_path + "/maps", data_path + "/items.json", data_path + "/npcs.json"),
      command_manager_(data_path + "/commands.json"),
      nlp_("http://localhost:3000/api/generate", data_path + "/llm_prompt.txt") {
  do_accept();
}

void server::run() { io_context_.run(); }

void server::join(chat_participant_ptr participant) {
    sessions_.insert(participant);
}

void server::leave(chat_participant_ptr participant) {
    auto session_ptr = std::dynamic_pointer_cast<mud::session>(participant);
    if (session_ptr && session_ptr->get_player()) {
        std::string username = session_ptr->get_player()->get_name();
        // std::string msg = "\033[90m" + username + " has left the game.\033[0m";
        std::string msg = utils::color::left(username + " has left the game.");
        utils::Logger::instance().log(username + " has left the game.");
        remove_player(username);
        broadcast(msg, participant);
    }
    sessions_.erase(participant);
}

void server::broadcast(const std::string &msg, chat_participant_ptr sender) {
    for (auto &participant : sessions_) {
        auto s = std::dynamic_pointer_cast<mud::session>(participant);
        if (s && s->is_logged_in() && participant != sender) {
            s->deliver(msg);
        }
    }
}

void server::broadcast_to_room(const std::string &msg,
                               std::shared_ptr<world::Room> room,
                               chat_participant_ptr sender) {
  for (auto &participant : sessions_) {
    if (participant != sender) {
      auto s = std::dynamic_pointer_cast<mud::session>(participant);
      if (s && s->is_logged_in() && s->get_player() &&
          s->get_player()->get_room() == room) {
        s->deliver(msg);
      }
    }
  }
}

std::shared_ptr<Player> server::add_player(const std::string &name) {
    if (players_.find(name) != players_.end()) {
        return nullptr;
    }
    auto player = std::make_shared<Player>(name);
    players_[name] = player;
    return player;
}

void server::remove_player(const std::string &name) {
    players_.erase(name);
}

std::shared_ptr<Player> server::get_player_by_name(const std::string &name) {
  auto it = players_.find(name);
  if (it != players_.end()) {
    return it->second;
  }
  return nullptr;
}

world::World &server::get_world() { return world_; }

const CommandManager &server::get_command_manager() const {
  return command_manager_;
}

NaturalLanguageProcessor &server::get_nlp() { return nlp_; }

boost::asio::thread_pool& server::get_thread_pool() { return thread_pool_; }

void server::do_accept() {
  acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
    if (!ec) {
      auto new_session =
          std::make_shared<session>(std::move(socket), *this);
      new_session->start();
    }
    do_accept();
  });
}
} // namespace mud
