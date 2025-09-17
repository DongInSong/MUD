#include "world/item_manager.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace mud::world {

using json = nlohmann::json;

ItemManager::ItemManager() {}

void ItemManager::load_items(const std::string& file_path) {
    std::ifstream f(file_path);
    json data = json::parse(f);

    for (const auto& item_data : data["items"]) {
        auto item = std::make_shared<Item>();
        item->id = item_data.at("id").get<std::string>();
        item->name = item_data.at("name").get<std::string>();
        item->description = item_data.at("description").get<std::string>();
        item->type = item_data.at("type").get<std::string>();
        items_[item->id] = item;
    }
}

std::shared_ptr<Item> ItemManager::get_item(const std::string& item_id) const {
    auto it = items_.find(item_id);
    if (it != items_.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace mud::world
