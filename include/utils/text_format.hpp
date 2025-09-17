#ifndef MUD_UTILS_TEXT_FORMAT_HPP
#define MUD_UTILS_TEXT_FORMAT_HPP

#include <string>
#include <vector>

namespace mud { namespace world { class Room; } }

namespace mud::utils::text_format {

struct ContentSection {
    std::string title;
    std::vector<std::string> items;
};

std::string create_boxed_message(
    const std::string& title,
    const std::string& description,
    const std::vector<ContentSection>& sections,
    int width = 80
);

std::string create_map_view(
    const world::Room& room,
    int player_x,
    int player_y
);

} // namespace mud::utils::text_format

#endif // MUD_UTILS_TEXT_FORMAT_HPP
