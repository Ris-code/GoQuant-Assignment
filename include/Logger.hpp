#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <mutex>
#include <fstream>

class Logger {
public:
    static Logger& getInstance();
    void log(const std::string& message);
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::ofstream log_file_;
    std::mutex mtx_;
};

#endif // LOGGER_HPP
