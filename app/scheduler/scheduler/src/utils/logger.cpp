#include "utils/logger.h"

#include <fmt/compile.h>
#include <fmt/ostream.h>

LOGGER_LEVEL &increaseVerbosity(LOGGER_LEVEL &l) {
    switch (l) {
    case LOGGER_LEVEL::FATAL:
        return l = LOGGER_LEVEL::ERROR;
    case LOGGER_LEVEL::ERROR:
        return l = LOGGER_LEVEL::WARNING;
    case LOGGER_LEVEL::WARNING:
        return l = LOGGER_LEVEL::INFO;
    case LOGGER_LEVEL::INFO:
        return l = LOGGER_LEVEL::DEBUG;
    case LOGGER_LEVEL::DEBUG:
        return l = LOGGER_LEVEL::TRACE;
    default:
        break;
    }
    return l;
}

void Logger::setVerbosity(LOGGER_LEVEL l) {
    getInstance().level = l;
}

LOGGER_LEVEL Logger::getVerbosity() {
    return getInstance().level;
}

Logger &Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(LOGGER_LEVEL l, std::string_view msg) {
    if (l <= level) {
        fmt::print(FMT_COMPILE("[{}]: {}\n"), l, msg);
    }
}
