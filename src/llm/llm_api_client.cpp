#include "llm/llm_api_client.hpp"
#include <cpr/cpr.h>
#include "utils/logger.hpp"
#include <iostream>
#include <algorithm>
#include <regex>

namespace mud {

LlmApiClient::LlmApiClient(const std::string& api_url) : api_url_(api_url) {}

std::string LlmApiClient::build_prompt(const std::string& user_input) {
    std::string prompt_text = 
    "You are a command parser for a MUD (Multi-User Dungeon) game.\n"
    "Your task is to convert the user's natural language input into a valid JSON object.\n"
    "\n"
    "Important:\n"
    "- Output must be in standard JSON format.\n"
    "- Use **double quotes**, not triple quotes or backticks.\n"
    "- Do not include markdown formatting or extra text. Only return the JSON.\n"
    "\n"
    "Available commands:\n"
    "MOVE, LOOK, ATTACK, GET, TALK, QUIT, CLEAR, INTERACT, CHAT\n"
    "\n"
    "Examples:\n"
    "Input: \"이동한다\"\n"
    "Output: {\"command\": \"MOVE\"}\n"
    "\n"
    "Input: \"동쪽으로 3걸음 이동해\"\n"
    "Output: {\"command\": \"MOVE\", \"direction\": \"EAST\", \"amount\": 3}\n"
    "\n"
    "Input: \"노인에게 말 걸어봐\"\n"
    "Output: {\"command\": \"TALK\", \"target\": \"노인\"}\n"
    "\n"
    "Input: \"이야기한다\"\n"
    "Output: {\"command\": \"TALK\"}\n"
    "\n"
    "Input: \"아이템을 주워\"\n"
    "Output: {\"command\": \"GET\"}\n"
    "\n"
    "Input: \"적을 공격해!\"\n"
    "Output: {\"command\": \"ATTACK\", \"target\": \"적\"}\n"
    "\n"
    // "Input: \"'안녕?' 이라고 말해\"\n"
    // "Output: {\"command\": \"SAY\", \"message\": \"안녕?\"}\n"
    // "\n"
    "Input: \"게임 종료\"\n"
    "Output: {\"command\": \"QUIT\"}\n"
    "\n"
    // "Input: \"동료에게 속삭여 '조심해'\"\n"
    // "Output: {\"command\": \"WHISPER\", \"target\": \"동료\", \"message\": \"조심해\"}\n"
    // "\n"
    "Input: \"포탈을 이용한다\"\n"
    "Output: {\"command\": \"INTERACT\"}\n"
    "\n"
    "Input: \"좌표 3,5로 이동\"\n"
    "Output: {\"command\": \"MOVE\", \"x\": 3, \"y\": 5}\n"
    "\n"
    "Now, convert the following user input to JSON format.\n"
    "User Input: " + user_input + "\n"
    "Output:";
    
    nlohmann::json payload = {
        // {"model", "llama3.1best"},
        {"model", "llama3.2:3b"},
        {"prompt", prompt_text},
        {"stream", false},
        {"options", {
            {"temperature", 0.0},
            {"num_predict", 128},
            {"stop", {"\n"}}
        }}
    };
    return payload.dump();
}

nlohmann::json LlmApiClient::parse_command(const std::string& user_input) {
    std::string payload = build_prompt(user_input);
    
    cpr::Response r = cpr::Post(cpr::Url{api_url_},
                                cpr::Body{payload},
                                cpr::Header{{"Content-Type", "application/json"}});

    if (r.status_code == 200) {
        try {
            std::string response_text = r.text;

            size_t first_brace = response_text.find('{');
            size_t last_brace = response_text.rfind('}');
            if (first_brace != std::string::npos && last_brace != std::string::npos && first_brace < last_brace) {
                std::string json_part = response_text.substr(first_brace, last_brace - first_brace + 1);
                try {
                    nlohmann::json response_json = nlohmann::json::parse(json_part);
                    if (response_json.contains("response")) {
                        std::string content = response_json["response"];
                        
                        size_t first_brace_content = content.find('{');
                        size_t last_brace_content = content.rfind('}');
                        
                        if (first_brace_content != std::string::npos && last_brace_content != std::string::npos && first_brace_content < last_brace_content) {
                            std::string command_json_str = content.substr(first_brace_content, last_brace_content - first_brace_content + 1);
                            
                            command_json_str = std::regex_replace(command_json_str, std::regex("\\\\\""), "\"");
                            command_json_str = std::regex_replace(command_json_str, std::regex("[“”]"), "\"");

                            try {
                                utils::Logger::instance().log("LLM Command JSON: " + command_json_str);
                                return nlohmann::json::parse(command_json_str);
                            } catch (const nlohmann::json::parse_error& e) {
                                std::cerr << "Inner JSON parse error: " << e.what() << std::endl;
                                std::cerr << "Content that failed to parse: " << command_json_str << std::endl;
                                return nullptr;
                            }
                        }
                    }
                } catch (const nlohmann::json::parse_error& e) {
                    std::cerr << "Outer JSON parse error: " << e.what() << std::endl;
                    std::cerr << "Content that failed to parse: " << json_part << std::endl;
                    return nullptr;
                }
            }
            return nullptr;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "LLM response JSON parse error: " << e.what() << std::endl;
            return nullptr;
        }
    } else {
        std::cerr << "LLM API request failed with status code: " << r.status_code << std::endl;
        std::cerr << "Response: " << r.text << std::endl;
        return nullptr;
    }
    return nullptr;
}

} // namespace mud
