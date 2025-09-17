#include "commands/command_handler.hpp"
#include "network/session.hpp"
#include "network/server.hpp"
#include "players/player.hpp"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include "utils/text_format.hpp"
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
  commands_["TALK"] = 
      std::bind(&CommandHandler::talk, this, std::placeholders::_1);
  commands_["GET"] = 
      std::bind(&CommandHandler::get, this, std::placeholders::_1);
  commands_["MAP"] =
      std::bind(&CommandHandler::show_map, this, std::placeholders::_1);
  commands_["CHAT"] = [this](const auto& args) { session_.toggle_chat_mode(); };
}

void CommandHandler::quit(const std::vector<std::string> &args) {
  session_.stop();
}

// Helper function to determine relative direction
std::string get_relative_direction(int player_x, int player_y, int target_x, int target_y) {
    std::string direction = "";
    if (target_y < player_y) direction += "북";
    else if (target_y > player_y) direction += "남";

    if (target_x < player_x) direction += "서";
    else if (target_x > player_x) direction += "동";

    if (direction.empty()) return "발밑";
    return direction + "쪽";
}

void CommandHandler::look(const std::vector<std::string> &args) {
  auto player = session_.get_player();
  if (!player || !player->get_room()) {
    session_.deliver(utils::color::error("You are lost in the void."));
    return;
  }
  auto room = player->get_room();
  int player_x = player->get_x();
  int player_y = player->get_y();
  int sight_radius = player->get_sight_radius();

  std::string title = room->get_name() + " (" + std::to_string(player_x) + ", " + std::to_string(player_y) + ")";
  std::string description = room->get_description();
  
  utils::text_format::ContentSection objects_section;
  objects_section.title = "주변 사물";
  
  utils::text_format::ContentSection portals_section;
  portals_section.title = "포탈";

  // 현재 타일 정보
  const auto& current_tile = room->get_tile(player_x, player_y);
  for (const auto& obj : current_tile.objects) {
      objects_section.items.push_back(obj.description + " (발밑)");
  }
  if (current_tile.portal) {
      portals_section.items.push_back(current_tile.portal->description + " (발밑)");
  }

  // 주변 시야 정보
  for (int y = player_y - sight_radius; y <= player_y + sight_radius; ++y) {
      for (int x = player_x - sight_radius; x <= player_x + sight_radius; ++x) {
          if (x == player_x && y == player_y) continue;
          if (x >= 0 && x < room->get_width() && y >= 0 && y < room->get_height()) {
              const auto& tile = room->get_tile(x, y);
              if (!tile.objects.empty() || tile.portal) {
                  std::string direction = get_relative_direction(player_x, player_y, x, y);
                  std::string coord_str = " (" + std::to_string(x) + "," + std::to_string(y) + ")";
                  for (const auto& obj : tile.objects) {
                      objects_section.items.push_back(direction + coord_str + "에 " + obj.description);
                  }
                  if (tile.portal) {
                      portals_section.items.push_back(direction + coord_str + "에 " + tile.portal->description);
                  }
              }
          }
      }
  }

  if (objects_section.items.empty() && portals_section.items.empty()) {
      description += "\n주변에 특별한 것은 보이지 않습니다.";
  }

  std::vector<utils::text_format::ContentSection> sections;
  if (!objects_section.items.empty()) {
      sections.push_back(objects_section);
  }
  if (!portals_section.items.empty()) {
      sections.push_back(portals_section);
  }

  session_.deliver(utils::text_format::create_boxed_message(title, description, sections));
}

void CommandHandler::move(int dx, int dy, int amount) {
  auto player = session_.get_player();
  if (!player || !player->get_room()) {
    session_.deliver(utils::color::error("이동할 수 없습니다."));
    return;
  }

  int initial_x = player->get_x();
  int initial_y = player->get_y();
  bool moved = false;

  for (int i = 0; i < amount; ++i) {
    auto room = player->get_room();
    int new_x = player->get_x() + dx;
    int new_y = player->get_y() + dy;

    if (new_x >= 0 && new_x < room->get_width() && new_y >= 0 &&
        new_y < room->get_height()) {
      player->set_location(room, new_x, new_y);
      moved = true;
    } else {
      session_.deliver(utils::color::error("그쪽으로는 더 이상 갈 수 없습니다."));
      break; // Stop moving if hit a wall
    }
  }

  if (moved) {
    session_.deliver(utils::color::color(utils::color::MOVE, "[ 이동 ] >>") + " 현재 위치: (" + std::to_string(player->get_x()) + ", " + std::to_string(player->get_y()) + ")");
    look_at_tile(&session_);
  }
}

void CommandHandler::move_to(const std::vector<std::string> &args) {
    if (args.empty()) {
        session_.deliver(utils::color::info("어디로 이동하시겠습니까? (예: 동쪽으로 3걸음 또는 3,5로 이동)"));
        return;
    }

    // 좌표 이동인지 방향 이동인지 판별
    bool is_coordinate_move = false;
    if (args.size() == 2) {
        try {
            std::stoi(args[0]);
            std::stoi(args[1]);
            is_coordinate_move = true;
        } catch (const std::exception&) {
            is_coordinate_move = false;
        }
    }

    if (is_coordinate_move) {
        int target_x = std::stoi(args[0]);
        int target_y = std::stoi(args[1]);
        auto player = session_.get_player();
        auto room = player->get_room();

        if (target_x >= 0 && target_x < room->get_width() && target_y >= 0 && target_y < room->get_height()) {
            player->set_location(room, target_x, target_y);
            session_.deliver(utils::color::color(utils::color::MOVE, "[ 이동 ] >>") + " 현재 위치: (" + std::to_string(player->get_x()) + ", " + std::to_string(player->get_y()) + ")");
            look_at_tile(&session_);
        } else {
            session_.deliver(utils::color::error("그곳으로는 이동할 수 없습니다."));
        }
    } else { // 방향 이동
        std::string direction = args[0];
        int amount = 1;
        if (args.size() > 1) {
            try {
                amount = std::stoi(args[1]);
            } catch (const std::exception&) {
                amount = 1; // Default to 1 if conversion fails
            }
        }

        if (direction == "NORTH") move(0, -1, amount);
        else if (direction == "SOUTH") move(0, 1, amount);
        else if (direction == "EAST") move(1, 0, amount);
        else if (direction == "WEST") move(-1, 0, amount);
        else {
            session_.deliver(utils::color::error("알 수 없는 방향입니다: " + direction));
        }
    }
}

void CommandHandler::say(const std::vector<std::string> &args) {
  auto player = session_.get_player();
  if (!player || !player->get_room()) {
    session_.deliver(
        utils::color::error("You are not in a room to speak."));
    return;
  }
  if (args.empty()) {
    session_.deliver(utils::color::info("무엇을 말하고 싶으신가요?"));
    session_.deliver(utils::color::info("사용법: /말하기 <메시지> 또는 /ㄴ <메시지> 또는 /ㅊ 으로 채팅 모드로 전환"));
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
    session_.deliver(utils::color::info("무엇을 외치고 싶으신가요?"));
    session_.deliver(utils::color::info("사용법: /외치기 <메시지> 또는 /ㅂ <메시지>"));
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
    session_.deliver(utils::color::info(
        "누구에게 무엇을 속삭이시겠습니까?"));
      session_.deliver(utils::color::info("사용법: /귓속말 <대상> <메시지> 또는 /ㅅ <대상> <메시지"));
    return;
  }

  std::string target_name = args[0];

  std::string message;
  for (size_t i = 1; i < args.size(); ++i) {
    message += args[i] + (i == args.size() - 1 ? "" : " ");
  }

  auto target_player = session_.get_server().get_player_by_name(target_name);

  if (!target_player) {
    session_.deliver(utils::color::error("대상 플레이어를 찾을 수 없습니다: " + target_name));
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
    session_.deliver(utils::color::error("상호작용할 수 있는 장소에 없습니다."));
    return;
  }
  
  auto room = player->get_room();
  const auto& tile = room->get_tile(player->get_x(), player->get_y());

  if (args.empty()) { // No target specified, interact with everything on the tile
    bool interacted = false;
    if (tile.objects.empty() && !tile.portal) {
        session_.deliver(utils::color::info("이곳에는 상호작용할 것이 없습니다."));
        return;
    }

    for (const auto& obj : tile.objects) {
        session_.deliver(utils::color::event(obj.name + "와(과) 상호작용합니다."));
        if (obj.type == "npc") {
            talk({obj.name});
        } else if (obj.type == "item") {
            get({obj.name});
        }
        interacted = true;
    }

    if (tile.portal) {
        auto target_room = session_.get_server().get_world().get_room(tile.portal->target_map);
        if (target_room) {
            player->set_location(target_room, tile.portal->target_x, tile.portal->target_y);
            session_.deliver("\n" + utils::color::system(target_room->get_name() + "에 도착했습니다."));
            handle("LOOK", {});
        } else {
            session_.deliver(utils::color::error("포탈이 작동하지 않는 것 같습니다."));
        }
        interacted = true;
    }
  } else { // Target specified
    std::string target_name = args[0];
    // Find the object in the room and interact with it
    // This part needs a way to find objects by name in the room.
    // For now, we'll just deliver a message.
    session_.deliver(utils::color::info(target_name + "와(과) 상호작용을 시도합니다. (미구현)"));
  }
}

void CommandHandler::talk(const std::vector<std::string> &args) {
    if (args.empty()) {
        // 인자 없이 "이야기한다"라고 입력한 경우, 현재 위치의 NPC와 대화 시도
        auto player = session_.get_player();
        if (!player || !player->get_room()) return;

        auto room = player->get_room();
        const auto& tile = room->get_tile(player->get_x(), player->get_y());
        
        std::string npc_name;
        for (const auto& obj : tile.objects) {
            if (obj.type == "npc") {
                npc_name = obj.name;
                break;
            }
        }

        if (!npc_name.empty()) {
            session_.deliver(utils::color::event(npc_name + "에게 말을 겁니다."));
            // 여기에 실제 대화 로직 추가 가능
        } else {
            session_.deliver(utils::color::info("주변에 대화할 상대가 없습니다."));
        }
        return;
    }
    
    // 특정 대상에게 말을 거는 경우
    session_.deliver(utils::color::event(args[0] + "에게 말을 겁니다."));
}

void CommandHandler::get(const std::vector<std::string> &args) {
    if (args.empty()) {
        session_.deliver(utils::color::info("무엇을 주우시겠습니까?"));
        return;
    }

    auto player = session_.get_player();
    if (!player || !player->get_room()) return;

    auto room = player->get_room();
    const auto& tile = room->get_tile(player->get_x(), player->get_y());
    std::string target_name = args[0];

    auto it = std::find_if(tile.objects.begin(), tile.objects.end(),
                           [&target_name](const world::Object& obj) {
                               return obj.name == target_name && obj.type == "item";
                           });

    if (it != tile.objects.end()) {
        const auto& obj = *it;
        auto item = session_.get_server().get_world().get_item_manager().get_item(obj.item_id);
        if (item) {
            player->add_item_to_inventory(*item);
            room->remove_object(player->get_x(), player->get_y(), obj.name);
            session_.deliver(utils::color::event(item->name + "을(를) 주웠습니다."));
        } else {
            session_.deliver(utils::color::error("아이템 정보를 찾을 수 없습니다."));
        }
    } else {
        session_.deliver(utils::color::info("여기에는 그런 아이템이 없습니다."));
    }
}

void CommandHandler::show_map(const std::vector<std::string> &args) {
    auto player = session_.get_player();
    if (!player || !player->get_room()) return;

    auto room = player->get_room();
    std::string map_item_id = "map_" + room->get_id();

    if (player->has_item(map_item_id)) {
        session_.deliver(utils::text_format::create_map_view(*room, player->get_x(), player->get_y()));
    } else {
        session_.deliver(utils::color::info("이 지역의 지도를 가지고 있지 않습니다."));
    }
}

} // namespace mud
