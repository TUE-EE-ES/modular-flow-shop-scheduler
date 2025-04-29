#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/utils/logger.hpp"

namespace {
std::string_view loggingColor(const fms::utils::LOGGER_LEVEL &l) {
    switch (l) {
    case fms::utils::LOGGER_LEVEL::CRITICAL:
    case fms::utils::LOGGER_LEVEL::ERROR:
        return "\033[31m";
    case fms::utils::LOGGER_LEVEL::WARNING:
        return "\033[33m";
    case fms::utils::LOGGER_LEVEL::INFO:
        return "\033[32m";
    case fms::utils::LOGGER_LEVEL::DEBUG:
        return "\033[34m";
    case fms::utils::LOGGER_LEVEL::TRACE:
        return "\033[90m";
    default:
        break;
    }
    return "";
}
} // namespace

namespace fms::utils {

LOGGER_LEVEL &increaseVerbosity(LOGGER_LEVEL &l) {
    switch (l) {
    case LOGGER_LEVEL::CRITICAL:
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

void Logger::setVerbosity(LOGGER_LEVEL l) { getInstance().level = l; }

LOGGER_LEVEL Logger::getVerbosity() { return getInstance().level; }

Logger &Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(LOGGER_LEVEL l, std::string_view msg) {
    if (l <= level) {
        logWithColor(l, msg);
    }
}

void Logger::logWithColor(LOGGER_LEVEL l, std::string_view msg) {
    fmt::print(FMT_COMPILE("{}[{}]: {}\033[m\n"), loggingColor(l), l, msg);
}

} // namespace fms::utils
