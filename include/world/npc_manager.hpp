#ifndef MUD_NPC_MANAGER_HPP
#define MUD_NPC_MANAGER_HPP

#include "world/npc.hpp"
#include <map>
#include <string>
#include <optional>

namespace mud::world {

class NpcManager {
public:
    bool load_npcs(const std::string& file_path);
    std::optional<Npc> get_npc(int npc_id) const;

private:
    std::map<int, Npc> npcs_;
};

} // namespace mud::world

#endif // MUD_NPC_MANAGER_HPP
