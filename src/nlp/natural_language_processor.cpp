#include "nlp/natural_language_processor.hpp"

namespace mud {

NaturalLanguageProcessor::NaturalLanguageProcessor(const std::string& llm_api_url)
    : llm_client_(llm_api_url) {}

ParsedCommand NaturalLanguageProcessor::parse(const std::string& input) {
    ParsedCommand result;
    nlohmann::json llm_response = llm_client_.parse_command(input);

    if (llm_response.is_null() || !llm_response.contains("command")) {
        result.command = "";
        result.args.push_back("아무 일도 하지 않았습니다.");
        return result;
    }

    result.command = llm_response.value("command", "");

    // "이동한다"와 같이 인자 없는 MOVE 명령어 처리
    if (result.command == "MOVE" && llm_response.size() == 1) {
        result.command = ""; // Command를 비워서 Session에서 시스템 메시지로 처리하도록 유도
        result.args.push_back("어디로 이동하시겠습니까?");
        return result;
    }

    // MOVE 명령어는 인자 순서가 중요하므로 특별히 처리
    if (result.command == "MOVE") {
        if (llm_response.contains("direction")) {
            result.args.push_back(llm_response["direction"]);
            if (llm_response.contains("amount")) {
                result.args.push_back(std::to_string(llm_response["amount"].get<int>()));
            }
        } else if (llm_response.contains("x") && llm_response.contains("y")) {
            result.args.push_back(std::to_string(llm_response["x"].get<int>()));
            result.args.push_back(std::to_string(llm_response["y"].get<int>()));
        }
    } else {
        // 다른 명령어들은 순서가 중요하지 않으므로 동적으로 처리
        for (auto& [key, value] : llm_response.items()) {
            if (key != "command") {
                if (value.is_string()) {
                    result.args.push_back(value.get<std::string>());
                } else if (value.is_number()) {
                    result.args.push_back(std::to_string(value.get<int>()));
                }
            }
        }
    }

    return result;
}

} // namespace mud
