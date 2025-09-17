#include "world/npc_manager.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

namespace mud::world {

    using json = nlohmann::json;

    bool NpcManager::load_npcs(const std::string& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            utils::Logger::instance().log("Failed to open NPC file: " + file_path);
            return false;
        }

        json npc_json;
        try {
            file >> npc_json;
        } catch (json::parse_error& e) {
            utils::Logger::instance().log("Failed to parse NPC JSON: " + std::string(e.what()));
            return false;
        }

        for (const auto& npc_data : npc_json) {
            Npc npc;
            npc.id = npc_data.at("id");
            npc.name = npc_data.at("name");
            
            const auto& dialogue_data = npc_data.at("dialogue");
            npc.dialogue.default_dialogue = dialogue_data.at("default");

            for (const auto& state_data : dialogue_data.at("states")) {
                DialogueState state;
                state.state = state_data.at("state");
                state.text = state_data.at("text");
                npc.dialogue.states.push_back(state);
            }
            
            npcs_[npc.id] = npc;
        }

        utils::Logger::instance().log("Loaded " + std::to_string(npcs_.size()) + " NPCs.");
        return true;
    }

    std::optional<Npc> NpcManager::get_npc(int npc_id) const {
        auto it = npcs_.find(npc_id);
        if (it != npcs_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

} // namespace mud::world
