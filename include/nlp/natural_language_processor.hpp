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
    NaturalLanguageProcessor(const std::string& llm_api_url, const std::string& prompt_file_path);
    ParsedCommand parse(const std::string& input);

private:
    LlmApiClient llm_client_;
    std::string base_prompt_;
};

} // namespace mud
