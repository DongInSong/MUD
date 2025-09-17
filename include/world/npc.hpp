#ifndef MUD_NPC_HPP
#define MUD_NPC_HPP

#include <string>
#include <vector>
#include <map>

namespace mud::world {

struct DialogueState {
    std::string state;
    std::string text;
};

struct Dialogue {
    std::string default_dialogue;
    std::vector<DialogueState> states;
};

struct Npc {
    int id;
    std::string name;
    Dialogue dialogue;
};

} // namespace mud::world

#endif // MUD_NPC_HPP
