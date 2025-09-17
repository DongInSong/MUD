#pragma once

#include "commands/command_handler.hpp"
#include "network/chat_participant.hpp"
#include "nlp/natural_language_processor.hpp" // ParsedCommand를 위해 추가
#include <boost/asio.hpp>
#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mud {
class server;
class Player;

using boost::asio::ip::tcp;

class session : public chat_participant,
                public std::enable_shared_from_this<session> {
public:
  session(tcp::socket socket, server &server);
  void start();
  void deliver(const std::string &msg) override;
  void stop();

  // Getters for CommandHandler
  std::shared_ptr<Player> get_player() const;
  server &get_server();
  bool is_logged_in() const;
  void toggle_chat_mode();

private:
  void do_read();
  void do_write();
  void handle_initial_input(const std::string &input);
  void handle_message(const std::string &msg);
  void handle_parsed_command(const ParsedCommand& parsed, const std::string& original_msg);
  void process_command(const std::string &input);

  tcp::socket socket_;
  server &server_;
  boost::asio::streambuf buffer_;
  std::deque<std::string> write_msgs_;
  std::shared_ptr<Player> player_;
  bool is_logged_in_ = false;
  bool isInChatMode_ = false;
  std::atomic<bool> closing_{false};
  std::string remote_endpoint_str_;
  CommandHandler command_handler_;
};
} // namespace mud
