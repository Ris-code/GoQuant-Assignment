#include "Config.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

Config Config::load(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file.");
    }
    
    json j;
    file >> j;
    
    Config config;
    config.api_key = j.at("api_key").get<std::string>();
    config.api_secret = j.at("api_secret").get<std::string>();
    config.websocket_url = j.at("websocket_url").get<std::string>();
    config.rest_url = j.at("rest_url").get<std::string>();
    config.websocket_port = j.at("websocket_port").get<int>();
    config.log_file = j.at("log_file").get<std::string>();
    
    return config;
}
