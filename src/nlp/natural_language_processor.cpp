#include "nlp/natural_language_processor.hpp"
#include <fstream>
#include <sstream>

namespace mud {

NaturalLanguageProcessor::NaturalLanguageProcessor(const std::string& llm_api_url, const std::string& prompt_file_path)
    : llm_client_(llm_api_url) {
    std::ifstream prompt_file(prompt_file_path);
    std::stringstream buffer;
    buffer << prompt_file.rdbuf();
    base_prompt_ = buffer.str();
}

ParsedCommand NaturalLanguageProcessor::parse(const std::string& input) {
    ParsedCommand result;
    std::string full_prompt = base_prompt_ + "\nUser Input: " + input + "\nOutput:";
    nlohmann::json llm_response = llm_client_.parse_command(full_prompt);

    if (llm_response.is_null() || !llm_response.contains("command")) {
        result.command = "";
        result.args.push_back("아무 일도 하지 않았습니다.");
        return result;
    }

    result.command = llm_response.value("command", "");

    if (result.command == "MOVE") {
        if (llm_response.contains("args")) { // Coordinate move
            for (const auto& arg : llm_response["args"]) {
                if (arg.is_number()) {
                    result.args.push_back(std::to_string(arg.get<int>()));
                } else if (arg.is_string()) {
                    result.args.push_back(arg.get<std::string>());
                }
            }
        } else if (llm_response.contains("direction")) { // Directional move
            result.args.push_back(llm_response["direction"]);
            if (llm_response.contains("amount")) {
                result.args.push_back(std::to_string(llm_response["amount"].get<int>()));
            }
        }
    } else if (llm_response.contains("args")) { // Other commands
        for (const auto& arg : llm_response["args"]) {
            if (arg.is_string()) {
                result.args.push_back(arg.get<std::string>());
            } else if (arg.is_number()) {
                result.args.push_back(std::to_string(arg.get<int>()));
            }
        }
    }

    return result;
}

} // namespace mud
