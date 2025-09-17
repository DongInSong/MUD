#ifndef MUD_ITEM_HPP
#define MUD_ITEM_HPP

#include <string>

namespace mud {

struct Item {
    std::string id;
    std::string name;
    std::string description;
    std::string type;
};

} // namespace mud

#endif // MUD_ITEM_HPP
