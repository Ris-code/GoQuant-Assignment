#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

struct Config {
    std::string api_key;
    std::string api_secret;
    std::string websocket_url;
    std::string rest_url;
    int websocket_port;
    std::string log_file;
    
    static Config load(const std::string& config_file);
};

#endif // CONFIG_HPP
