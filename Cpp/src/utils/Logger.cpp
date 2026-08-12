#include "utils/Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace ImageCut {
namespace Utils {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void Logger::init(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        std::filesystem::path p(logFilePath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        m_logFile.open(logFilePath, std::ios::out | std::ios::app);
        m_initialized = m_logFile.is_open();
    } catch (...) {
        m_initialized = false;
    }
}

static std::string levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
    }
    return "INFO";
}

void Logger::log(LogLevel level, const std::string& message, const char* file, int line) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");

    std::stringstream formatted;
    formatted << "[" << ss.str() << "] [" << levelToString(level) << "]";
    if (file) {
        std::string filename = std::filesystem::path(file).filename().string();
        formatted << " [" << filename << ":" << line << "]";
    }
    formatted << " " << message;

    std::string str = formatted.str();
    std::cout << str << std::endl;

    if (m_initialized && m_logFile.is_open()) {
        m_logFile << str << std::endl;
        m_logFile.flush();
    }
}

void Logger::debug(const std::string& msg, const char* file, int line) {
    log(LogLevel::Debug, msg, file, line);
}

void Logger::info(const std::string& msg, const char* file, int line) {
    log(LogLevel::Info, msg, file, line);
}

void Logger::warning(const std::string& msg, const char* file, int line) {
    log(LogLevel::Warning, msg, file, line);
}

void Logger::error(const std::string& msg, const char* file, int line) {
    log(LogLevel::Error, msg, file, line);
}

} // namespace Utils
} // namespace ImageCut
