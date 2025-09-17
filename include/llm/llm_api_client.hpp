#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace mud {

class LlmApiClient {
public:
    LlmApiClient(const std::string& api_url);
    nlohmann::json parse_command(const std::string& user_input);

private:
    std::string api_url_;
    std::string build_prompt(const std::string& user_input);
};

} // namespace mud
