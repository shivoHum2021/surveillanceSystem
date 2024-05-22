#ifndef LOGGER_H
#define LOGGER_H

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <string>
#include <sstream>

#define LOG_INFO(message) { \
    std::ostringstream oss; \
    oss << __FUNCTION__ << ":" << __LINE__ << " - " << message; \
    Logger::log(Logger::INFO, oss.str()); \
}
#define LOG_DEBUG(message) { \
    std::ostringstream oss; \
    oss << __FUNCTION__ << ":" << __LINE__ << " - " << message; \
    Logger::log(Logger::DEBUG, oss.str()); \
}
#define LOG_WARN(message) { \
    std::ostringstream oss; \
    oss << __FUNCTION__ << ":" << __LINE__ << " - " << message; \
    Logger::log(Logger::WARN, oss.str()); \
}
#define LOG_ERROR(message) { \
    std::ostringstream oss; \
    oss << __FUNCTION__ << ":" << __LINE__ << " - " << message; \
    Logger::log(Logger::ERROR, oss.str()); \
}
#define LOG_TRACE(message) { \
    std::ostringstream oss; \
    oss << __FUNCTION__ << ":" << __LINE__ << " - " << message; \
    Logger::log(Logger::TRACE, oss.str()); \
}
#define LOG_FATAL(message) { \
    std::ostringstream oss; \
    oss << __FUNCTION__ << ":" << __LINE__ << " - " << message; \
    Logger::log(Logger::FATAL, oss.str()); \
}
class Logger {
public:
    enum LogLevel {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };

    static void log(LogLevel level, const std::string& message);
private:
    static log4cplus::Logger& getLogger();
};

#endif // LOGGER_H
