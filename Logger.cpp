#include "Logger.hpp"
#include <log4cplus/configurator.h>

void Logger::log(LogLevel level, const std::string &message)
{
    log4cplus::Logger logger = getLogger();

    switch (level)
    {
    case TRACE:
        LOG4CPLUS_TRACE(logger, message);
        break;
    case DEBUG:
        LOG4CPLUS_DEBUG(logger, message);
        break;
    case INFO:
        LOG4CPLUS_INFO(logger, message);
        break;
    case WARN:
        LOG4CPLUS_WARN(logger, message);
        break;
    case ERROR:
        LOG4CPLUS_ERROR(logger, message);
        break;
    case FATAL:
        LOG4CPLUS_FATAL(logger, message);
        break;
    }
}

log4cplus::Logger &Logger::getLogger()
{
    static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("ApplicationLogger"));
    return logger;
}
