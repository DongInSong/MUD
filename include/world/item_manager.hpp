#ifndef MUD_ITEM_MANAGER_HPP
#define MUD_ITEM_MANAGER_HPP

#include "world/item.hpp"
#include <string>
#include <unordered_map>
#include <memory>

namespace mud::world {

class ItemManager {
public:
    ItemManager();
    void load_items(const std::string& file_path);
    std::shared_ptr<Item> get_item(const std::string& item_id) const;

private:
    std::unordered_map<std::string, std::shared_ptr<Item>> items_;
};

} // namespace mud::world

#endif // MUD_ITEM_MANAGER_HPP
