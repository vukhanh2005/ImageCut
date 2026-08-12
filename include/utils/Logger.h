#ifndef IMAGECUT_LOGGER_H
#define IMAGECUT_LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <memory>

namespace ImageCut {
namespace Utils {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& getInstance();

    void init(const std::string& logFilePath);
    void log(LogLevel level, const std::string& message, const char* file = nullptr, int line = 0);

    void debug(const std::string& msg, const char* file = nullptr, int line = 0);
    void info(const std::string& msg, const char* file = nullptr, int line = 0);
    void warning(const std::string& msg, const char* file = nullptr, int line = 0);
    void error(const std::string& msg, const char* file = nullptr, int line = 0);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream m_logFile;
    std::mutex m_mutex;
    bool m_initialized = false;
};

} // namespace Utils
} // namespace ImageCut

#define LOG_DEBUG(msg) ImageCut::Utils::Logger::getInstance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) ImageCut::Utils::Logger::getInstance().info(msg, __FILE__, __LINE__)
#define LOG_WARN(msg) ImageCut::Utils::Logger::getInstance().warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) ImageCut::Utils::Logger::getInstance().error(msg, __FILE__, __LINE__)

#endif // IMAGECUT_LOGGER_H
