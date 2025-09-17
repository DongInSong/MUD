#pragma once

#include "llm/llm_api_client.hpp"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace mud {

struct ParsedCommand {
    std::string command;
    std::vector<std::string> args;
};

class NaturalLanguageProcessor {
public:
    NaturalLanguageProcessor(const std::string& llm_api_url);
    ParsedCommand parse(const std::string& input);

private:
    LlmApiClient llm_client_;
};

} // namespace mud
