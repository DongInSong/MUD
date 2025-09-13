#include "commands/command_handler.hpp"
#include "network/session.hpp"
#include "network/server.hpp"
#include "players/player.hpp"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include "world/room.hpp"

namespace mud {

// Helper function from session.cpp
void look_at_tile(session *s);

CommandHandler::CommandHandler(session &s) : session_(s) { setup_commands(); }

void CommandHandler::handle(const std::string &command,
                            const std::vector<std::string> &args) {
  auto it = commands_.find(command);
  if (it != commands_.end()) {
    it->second(args);
  } else {
    session_.deliver("Command not implemented: " + command);
  }
}

void CommandHandler::setup_commands() {
  commands_["QUIT"] =
      std::bind(&CommandHandler::quit, this, std::placeholders::_1);
  commands_["LOOK"] =
      std::bind(&CommandHandler::look, this, std::placeholders::_1);
  commands_["NORTH"] = [this](const auto &args) { move(0, -1); };
  commands_["SOUTH"] = [this](const auto &args) { move(0, 1); };
  commands_["EAST"] = [this](const auto &args) { move(1, 0); };
  commands_["WEST"] = [this](const auto &args) { move(-1, 0); };
  commands_["MOVE"] =
      std::bind(&CommandHandler::move_to, this, std::placeholders::_1);
  commands_["SAY"] =
      std::bind(&CommandHandler::say, this, std::placeholders::_1);
  commands_["SHOUT"] =
      std::bind(&CommandHandler::shout, this, std::placeholders::_1);
  commands_["WHISPER"] =
      std::bind(&CommandHandler::whisper, this, std::placeholders::_1);
  commands_["CLEAR"] =
      std::bind(&CommandHandler::clear, this, std::placeholders::_1);
  commands_["INTERACT"] =
      std::bind(&CommandHandler::interact, this, std::placeholders::_1);
}

void CommandHandler::quit(const std::vector<std::string> &args) {
  session_.stop();
}

void CommandHandler::look(const std::vector<std::string> &args) {
  auto player = session_.get_player();
  if (!player || !player->get_room()) {
    session_.deliver(utils::color::system("You are lost in the void."));
    return;
  }
  auto room = player->get_room();
  session_.deliver(
      "========================================");
  session_.deliver(
      room->get_name() + " (" + std::to_string(player->get_x()) + ", " +
      std::to_string(player->get_y()) + ")");
  session_.deliver(room->get_description());
  look_at_tile(&session_);
  session_.deliver(
      "========================================");
}

void CommandHandler::move(int dx, int dy) {
  auto player = session_.get_player();
  if (!player || !player->get_room()) {
    session_.deliver(utils::color::system("You can't move."));
    return;
  }
  auto room = player->get_room();
  int new_x = player->get_x() + dx;
  int new_y = player->get_y() + dy;

  if (new_x >= 0 && new_x < room->get_width() && new_y >= 0 &&
      new_y < room->get_height()) {
    player->set_location(room, new_x, new_y);
    session_.deliver(utils::color::tag(
        "move", utils::color::MOVE,
        "You moved to (" + std::to_string(new_x) + ", " +
            std::to_string(new_y) + ")."));
    look_at_tile(&session_);
  } else {
    session_.deliver(utils::color::system("You can't go that way."));
  }
}

void CommandHandler::move_to(const std::vector<std::string> &args) {
  if (args.empty()) {
    session_.deliver(
        utils::color::system("Move to where? (e.g., /m 5,5)"));
    return;
  }
  std::string coords = args[0];
  size_t comma_pos = coords.find(',');
  if (comma_pos == std::string::npos) {
    session_.deliver(
        utils::color::system("Invalid format. Use x,y (e.g., /m 5,5)"));
    return;
  }
  try {
    int x = std::stoi(coords.substr(0, comma_pos));
    int y = std::stoi(coords.substr(comma_pos + 1));

    auto player = session_.get_player();
    auto room = player->get_room();
    if (x >= 0 && x < room->get_width() && y >= 0 && y < room->get_height()) {
      player->set_location(room, x, y);
      session_.deliver(utils::color::move(
          "You moved to (" + std::to_string(x) + ", " +
              std::to_string(y) + ")."));
      look_at_tile(&session_);
    } else {
      session_.deliver(utils::color::system("You can't move there."));
    }
  } catch (const std::exception &) {
    session_.deliver(utils::color::system("Invalid coordinates."));
  }
}

void CommandHandler::say(const std::vector<std::string> &args) {
  auto player = session_.get_player();
  if (!player || !player->get_room()) {
    session_.deliver(
        utils::color::system("You are not in a room to speak."));
    return;
  }
  if (args.empty()) {
    session_.deliver(utils::color::system("What do you want to say?"));
    return;
  }

  std::string message;
  for (size_t i = 0; i < args.size(); ++i) {
    message += args[i] + (i == args.size() - 1 ? "" : " ");
  }
  std::string formatted_message = utils::color::say(player->get_name() + ": " + message);
  utils::Logger::instance().log("say: " + player->get_name() + ": " + message);
  session_.deliver(formatted_message);
  session_.get_server().broadcast_to_room(formatted_message, player->get_room(),
                                          session_.shared_from_this());
}

void CommandHandler::shout(const std::vector<std::string> &args) {
  auto player = session_.get_player();
  if (!player) return;
  if (args.empty()) {
    session_.deliver(utils::color::system("What do you want to shout?"));
    return;
  }
  std::string message;
  for (size_t i = 0; i < args.size(); ++i) {
    message += args[i] + (i == args.size() - 1 ? "" : " ");
  }
  std::string formatted_message = utils::color::shout(
      player->get_name() + ": " + message);
  utils::Logger::instance().log("shout: " + player->get_name() + ": " + message);
  session_.get_server().broadcast(formatted_message);
}

void CommandHandler::whisper(const std::vector<std::string> &args) {
  auto player = session_.get_player();
  if (!player)
    return;
  if (args.size() < 2) {
    session_.deliver(utils::color::system(
        "Who do you want to whisper to and what? (e.g., /w <player> <msg>)"));
    return;
  }

  std::string target_name = args[0];

  std::string message;
  for (size_t i = 1; i < args.size(); ++i) {
    message += args[i] + (i == args.size() - 1 ? "" : " ");
  }

  auto target_player = session_.get_server().get_player_by_name(target_name);

  if (!target_player) {
    session_.deliver(utils::color::system("Player not found: " + target_name));
    return;
  }

  std::string to_target_msg =
      utils::color::whisper(player->get_name() + ": " + message);
  target_player->send_message(to_target_msg);

  std::string to_self_msg =
      utils::color::whisper("To " + target_name + ": " + message);
  session_.deliver(to_self_msg);
}

void CommandHandler::clear(const std::vector<std::string> &args) {
  // ANSI escape code to clear screen and move cursor to top-left
  session_.deliver("\033[2J\033[H");
}

void CommandHandler::interact(const std::vector<std::string> &args) {
  auto player = session_.get_player();
  if (!player || !player->get_room()) {
    session_.deliver(utils::color::system("You are not in a room to interact."));
    return;
  }
  
  auto room = player->get_room();
  int x = player->get_x();
  int y = player->get_y();
  const auto &tile = room->get_tile(x, y);

  bool did_interact = false;

  if (tile.objects.empty() && !tile.portal) {
    session_.deliver(utils::color::system("There is nothing to interact with here."));
    return;
  }

  for (const auto &obj : tile.objects) {
    session_.deliver(utils::color::event("You interact with " + obj.name + "."));
    if (obj.type == "npc") {
      session_.deliver(utils::color::event(obj.name + " says: Hello there!"));
      // Add NPC interaction logic here
    } else if (obj.type == "item") {
      session_.deliver(utils::color::event("You pick up the " + obj.name + "."));
      // Add item pickup logic here
    } else {
      session_.deliver(utils::color::event("You examine the " + obj.name + ": " + obj.description));
    }
    did_interact = true;
    // Add more interaction logic here
  }

  if (tile.portal) {
    // session_.deliver(utils::color::portal("You use the portal."));

    // player location to portal target
    auto target_room = session_.get_server().get_world().get_room(tile.portal->target_map);
    if (target_room) {
        player->set_location(target_room, tile.portal->target_x, tile.portal->target_y);
        session_.deliver("\n" + utils::color::system("You arrive at " + target_room->get_name() + "." + 
            " (" + std::to_string(tile.portal->target_x) + ", " + std::to_string(tile.portal->target_y) + ")"));
        // look_at_tile(&session_);
        // use look command to show room info
        handle({"LOOK"}, {});
    } else {
        session_.deliver(utils::color::system("The portal seems to be malfunctioning."));
    }
    
    did_interact = true;
  }

  if (!did_interact) {
    session_.deliver(utils::color::system("There is nothing to interact with here."));
  }
}
} // namespace mud
