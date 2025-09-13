#pragma once

#include "commands/command_manager.hpp"
#include "network/chat_participant.hpp"
#include "players/player.hpp"
#include "world/world.hpp"
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace mud {
class session;

using boost::asio::ip::tcp;

class server {
public:
  server(boost::asio::io_context &io_context, const tcp::endpoint &endpoint,
         const std::string &data_path);
  void run();

  void join(chat_participant_ptr participant);
  void leave(chat_participant_ptr participant);
  void broadcast(const std::string &msg, chat_participant_ptr sender = nullptr);
  void broadcast_to_room(const std::string &msg,
                         std::shared_ptr<world::Room> room,
                         chat_participant_ptr sender);

  std::shared_ptr<Player> add_player(const std::string &name);
  void remove_player(const std::string &name);
  std::shared_ptr<Player> get_player_by_name(const std::string &name);

  world::World &get_world();
  const CommandManager &get_command_manager() const;

private:
  void do_accept();

  boost::asio::io_context &io_context_;
  tcp::acceptor acceptor_;
  std::set<chat_participant_ptr> sessions_;
  std::map<std::string, std::shared_ptr<Player>> players_;
  world::World world_;
  CommandManager command_manager_;
};
} // namespace mud
