#include "utils/text_format.hpp"
#include "utils/color.hpp"
#include "world/room.hpp"
#include <sstream>
#include <vector>
#include <string>

namespace mud::utils::text_format {

// Function to calculate the display width of a string (Korean characters as 2, others as 1)
int get_display_width(const std::string& str) {
    int width = 0;
    for (char c : str) {
        if ((c & 0xC0) != 0x80) { // Check if it is a starting byte of a UTF-8 character
            width += (static_cast<unsigned char>(c) >= 0x80) ? 2 : 1;
        }
    }
    return width;
}

// Function to wrap text to a specific width considering display width
std::vector<std::string> wrap_text(const std::string& text, int max_width) {
    std::vector<std::string> lines;
    std::string current_line;
    std::stringstream ss(text);
    std::string word;

    while (ss >> word) {
        if (get_display_width(current_line) + get_display_width(word) + 1 > max_width) {
            lines.push_back(current_line);
            current_line = word;
        } else {
            if (!current_line.empty()) {
                current_line += " ";
            }
            current_line += word;
        }
    }
    if (!current_line.empty()) {
        lines.push_back(current_line);
    }
    return lines;
}

std::string create_boxed_message(
    const std::string& title,
    const std::string& description,
    const std::vector<ContentSection>& sections,
    int width
) {
    std::stringstream box;
    int inner_width = width - 4;

    const std::string H_BAR = "═";
    std::string top_bottom_border;
    for (int i = 0; i < width - 2; ++i) {
        top_bottom_border += H_BAR;
    }

    // Top border
    box << "╔" << top_bottom_border << "╗\n";

    // Title
    int title_width = get_display_width(title);
    int title_padding = (inner_width - title_width) / 2;
    box << "║ " << std::string(title_padding, ' ') << color::color(color::BOLD_WHITE, title) << std::string(inner_width - title_width - title_padding, ' ') << " ║\n";

    // Separator
    box << "╠" << top_bottom_border << "╣\n";

    // Description
    auto wrapped_desc = wrap_text(description, inner_width);
    for (const auto& line : wrapped_desc) {
        box << "║ " << color::color(color::WHITE, line) << std::string(inner_width - get_display_width(line), ' ') << " ║\n";
    }

    // Content Sections
    if (!sections.empty()) {
        box << "║ " << std::string(inner_width, ' ') << " ║\n"; // Empty line before sections
        for (const auto& section : sections) {
            if (!section.items.empty()) {
                //if portal == color purple
                if (section.title == "포탈") {
                    box << "║ " << color::color(color::PORTAL, section.title) << std::string(inner_width - get_display_width(section.title), ' ') << " ║\n";
                } else {
                    box << "║ " << color::color(color::YELLOW, section.title) << std::string(inner_width - get_display_width(section.title), ' ') << " ║\n";
                }
                for (const auto& item : section.items) {
                    auto wrapped_item = wrap_text(item, inner_width - 2); // Indent
                     for (const auto& line : wrapped_item) {
                        box << "║   " << line << std::string(inner_width - get_display_width(line) - 2, ' ') << " ║\n";
                    }
                }
            }
        }
    }

    // Bottom border
    box << "╚" << top_bottom_border << "╝\n";

    return box.str();
}

std::string create_map_view(
    const world::Room& room,
    int player_x,
    int player_y
) {
    std::stringstream map_ss;
    int width = room.get_width() * 3 + 4; // 3 characters per tile + borders
    std::string title = room.get_name() + " 지도";

        const std::string H_BAR = "═";
    std::string top_bottom_border;
    for (int i = 0; i < width - 2; ++i) {
        top_bottom_border += H_BAR;
    }

    map_ss << "╔" << top_bottom_border << "╗\n";
    int title_width = get_display_width(title);
    int title_padding = (width - 4 - title_width) / 2;
    map_ss << "║ " << std::string(title_padding, ' ') << color::color(color::BOLD_WHITE, title) << std::string(width - 4 - title_width - title_padding, ' ') << " ║\n";
    map_ss << "╠" << top_bottom_border << "╣\n";

    for (int y = 0; y < room.get_height(); ++y) {
        map_ss << "║ ";
        for (int x = 0; x < room.get_width(); ++x) {
            if (x == player_x && y == player_y) {
                map_ss << color::color(color::BOLD_WHITE, " @ ");
            } else {
                const auto& tile = room.get_tile(x, y);
                if (tile.portal) {
                    map_ss << color::color(color::PORTAL, " # ");
                } else if (!tile.objects.empty()) {
                    map_ss << color::color(color::YELLOW, " * ");
                } else {
                    map_ss << " . ";
                }
            }
        }
        map_ss << " ║\n";
    }

    map_ss << "╚" << top_bottom_border << "╝\n";
    return map_ss.str();
}

} // namespace mud::utils::text_format
