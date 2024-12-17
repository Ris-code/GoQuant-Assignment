#include "Logger.hpp"
#include "Config.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

Logger::Logger() {
    Config config = Config::load("config.json");
    log_file_.open(config.log_file, std::ios::app);
    if (!log_file_.is_open()) {
        throw std::runtime_error("Cannot open log file.");
    }
}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto now = std::chrono::system_clock::now();
    auto itt = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&itt);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " | " << message << "\n";
    log_file_ << oss.str();
    log_file_.flush();
}
