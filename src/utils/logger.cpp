#include "utils/logger.hpp"
#include <sstream>

namespace mud::utils {

Logger::Logger() {
    log_file_.open("server.log", std::ios_base::app);
    if (!log_file_.is_open()) {
        std::cerr << "Failed to open log file." << std::endl;
    }
}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::log(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::string timed_message = "[" + ss.str() + "] " + message;
    
    std::cout << timed_message << std::endl;
    if (log_file_.is_open()) {
        log_file_ << timed_message << std::endl;
    }
}

} // namespace mud::utils
