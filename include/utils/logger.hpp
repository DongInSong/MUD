#ifndef MUD_LOGGER_HPP
#define MUD_LOGGER_HPP

#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace mud::utils {

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    void log(const std::string& message);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    std::ofstream log_file_;
    std::mutex mutex_;
};

} // namespace mud::utils

#endif // MUD_LOGGER_HPP
