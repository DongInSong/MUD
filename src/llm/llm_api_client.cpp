#include "llm/llm_api_client.hpp"
#include <cpr/cpr.h>
#include "utils/logger.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <regex>

namespace mud {

LlmApiClient::LlmApiClient(const std::string& api_url) : api_url_(api_url) {
    std::ifstream config_file("data/llm_config.json");
    if (config_file.is_open()) {
        nlohmann::json config_json;
        config_file >> config_json;
        model_ = config_json["model"];
    } else {
        utils::Logger::instance().log("Could not open llm_config.json, using default model.");
        model_ = "llama3.2:3b";
    }
}

nlohmann::json LlmApiClient::parse_command(const std::string& full_prompt) {
    nlohmann::json payload = {
        {"model", model_},
        {"prompt", full_prompt},
        {"stream", false},
        {"format", "json"}
    };
    
    cpr::Response r = cpr::Post(cpr::Url{api_url_},
                                cpr::Body{payload.dump()},
                                cpr::Header{{"Content-Type", "application/json; charset=utf-8"}});

    if (r.status_code == 200) {
        try {
            nlohmann::json response_json = nlohmann::json::parse(r.text);
            if (response_json.contains("response")) {
                std::string content = response_json["response"];
                try {
                    utils::Logger::instance().log("LLM Command JSON: " + content);
                    return nlohmann::json::parse(content);
                } catch (const nlohmann::json::parse_error& e) {
                    utils::Logger::instance().log("Inner JSON parse error: " + std::string(e.what()));
                    utils::Logger::instance().log("Content that failed to parse: " + content);
                    return nullptr;
                }
            }
            return nullptr;
        } catch (const nlohmann::json::parse_error& e) {
            utils::Logger::instance().log("Outer JSON parse error: " + std::string(e.what()));
            utils::Logger::instance().log("Content that failed to parse: " + r.text);
            return nullptr;
        }
    } else {
        utils::Logger::instance().log("LLM API request failed with status code: " + std::to_string(r.status_code));
        utils::Logger::instance().log("Response: " + r.text);
        return nullptr;
    }
    return nullptr;
}

} // namespace mud
