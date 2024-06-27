#ifndef LOGGER_H
#define LOGGER_H

#include <fmt/compile.h>
#include <string_view>

/**
 * @brief Logger levels.
 * @details The levels are extracted from https://stackoverflow.com/a/2031209/4005637
 */
enum class LOGGER_LEVEL {
    /// Error that is forcing a shutdown
    CRITICAL,

    /// Error to the current operation
    ERROR,

    /// Anything that can cause oddities but that can be recovered
    WARNING,

    /// Generally useful information to log
    INFO,

    /// Information that is diagnostically helpful to people more than just developers
    DEBUG,

    /// Only when I would be "tracing" the code and trying to find one part of a function
    TRACE
};

template <> struct fmt::formatter<LOGGER_LEVEL> : formatter<std::string_view> {
    using BaseFormatter = formatter<std::string_view>;
    template <typename FormatContext>
    auto format(const LOGGER_LEVEL level, FormatContext &ctx) -> decltype(ctx.out()) {
        switch (level) {
        case LOGGER_LEVEL::CRITICAL:
            return BaseFormatter::format("CRITICAL", ctx);
        case LOGGER_LEVEL::ERROR:
            return BaseFormatter::format("ERROR", ctx);
        case LOGGER_LEVEL::WARNING:
            return BaseFormatter::format("WARNING", ctx);
        case LOGGER_LEVEL::INFO:
            return BaseFormatter::format("INFO", ctx);
        case LOGGER_LEVEL::DEBUG:
            return BaseFormatter::format("DEBUG", ctx);
        case LOGGER_LEVEL::TRACE:
            return BaseFormatter::format("TRACE", ctx);
        }
        return ctx.out();
    }
};

LOGGER_LEVEL& increaseVerbosity(LOGGER_LEVEL& l);

class Logger
{
public:
    static void setVerbosity(LOGGER_LEVEL l);

    static LOGGER_LEVEL getVerbosity();

    static Logger & getInstance();
    void log(LOGGER_LEVEL l, std::string_view msg);

    template <typename... Args>
    void log(LOGGER_LEVEL l, fmt::format_string<Args...> &&format_str, Args &&...args) {
        if (l <= level) {
            fmt::print(FMT_COMPILE("[{}]: {}\n"),
                       l,
                       fmt::format(std::forward<fmt::format_string<Args...>>(format_str),
                                   std::forward<Args>(args)...));
        }
    }

    template <LOGGER_LEVEL l, typename... Args>
    void log(fmt::format_string<Args...> &&format_str, Args &&...args) {
        if (l <= level) {
            fmt::print(FMT_COMPILE("[{}]: {}\n"),
                       l,
                       fmt::format(std::forward<fmt::format_string<Args...>>(format_str),
                                   std::forward<Args>(args)...));
        }
    }

    template <LOGGER_LEVEL l,
              typename CompiledFormat,
              typename... Args,
              std::enable_if_t<fmt::detail::is_compiled_string<CompiledFormat>::value, int> = 0>
    void log(CompiledFormat &&cf, Args &&...args) {
        if (l <= level) {
            fmt::print(FMT_COMPILE("[{}]: {}\n"),
                       l,
                       fmt::format(std::forward<CompiledFormat>(cf), std::forward<Args>(args)...));
        }
    }

    Logger(const Logger &) = delete;
    Logger(Logger &&) = default;
    void operator=(const Logger &) = delete;
    Logger& operator=(Logger &&) = default;

    [[nodiscard]] static inline LOGGER_LEVEL getLevel() {
        return getInstance().level;
    }
private:

    Logger() = default;
    ~Logger() = default;
    LOGGER_LEVEL level = LOGGER_LEVEL::CRITICAL;
};

inline void LOG(std::string_view msg) { Logger::getInstance().log(LOGGER_LEVEL::INFO, msg); }

inline void LOG(LOGGER_LEVEL l, std::string_view msg) { Logger::getInstance().log(l, msg); }

template <LOGGER_LEVEL l = LOGGER_LEVEL::INFO, typename... Args>
inline constexpr void LOG(fmt::format_string<Args...> &&cf, Args &&...args) {
    Logger::getInstance().log<l>(std::forward<fmt::format_string<Args...>>(cf),
                                 std::forward<Args>(args)...);
}

template <LOGGER_LEVEL l = LOGGER_LEVEL::INFO,
          typename CompiledFormat,
          typename... Args,
          std::enable_if_t<fmt::detail::is_compiled_string<CompiledFormat>::value, int> = 0>
inline constexpr void LOG(CompiledFormat &&cf, Args &&...args) {
    Logger::getInstance().log<l>(std::forward<CompiledFormat>(cf), std::forward<Args>(args)...);
}

template <typename... Args>
inline constexpr void
LOG(LOGGER_LEVEL l, fmt::format_string<Args...> &&format_str, Args &&...args) {
    Logger::getInstance().log(
            l, std::forward<fmt::format_string<Args...>>(format_str), std::forward<Args>(args)...);
}

template <typename CompiledFormat,
          typename... Args,
          std::enable_if_t<fmt::detail::is_compiled_string<CompiledFormat>::value, int> = 0>
inline constexpr void LOG(LOGGER_LEVEL l, CompiledFormat &&cf, Args &&...args) {
    Logger::getInstance().log(l, std::forward<CompiledFormat>(cf), std::forward<Args>(args)...);
}

template <typename... Args>
inline constexpr void LOG_C(fmt::format_string<Args...> &&format_str, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::CRITICAL>(
            std::forward<fmt::format_string<Args...>>(format_str), std::forward<Args>(args)...);
}

template <typename CompiledFormat,
          typename... Args,
          std::enable_if_t<fmt::detail::is_compiled_string<CompiledFormat>::value, int> = 0>
inline constexpr void LOG_C(CompiledFormat &&cf, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::CRITICAL>(std::forward<CompiledFormat>(cf),
                                                   std::forward<Args>(args)...);
}

template <typename... Args>
inline constexpr void LOG_E(fmt::format_string<Args...> &&format_str, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::ERROR>(
            std::forward<fmt::format_string<Args...>>(format_str), std::forward<Args>(args)...);
}

template <typename CompiledFormat,
          typename... Args,
          std::enable_if_t<fmt::detail::is_compiled_string<CompiledFormat>::value, int> = 0>
inline constexpr void LOG_E(CompiledFormat &&cf, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::ERROR>(std::forward<CompiledFormat>(cf),
                                                   std::forward<Args>(args)...);
}

template <typename... Args>
inline constexpr void LOG_W(fmt::format_string<Args...> &&format_str, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::WARNING>(
            std::forward<fmt::format_string<Args...>>(format_str), std::forward<Args>(args)...);
}

template <typename CompiledFormat,
          typename... Args,
          std::enable_if_t<fmt::detail::is_compiled_string<CompiledFormat>::value, int> = 0>
inline constexpr void LOG_W(CompiledFormat &&cf, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::WARNING>(std::forward<CompiledFormat>(cf),
                                                     std::forward<Args>(args)...);
}

template <typename... Args>
inline constexpr void LOG_I(fmt::format_string<Args...> &&format_str, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::INFO>(
            std::forward<fmt::format_string<Args...>>(format_str), std::forward<Args>(args)...);
}

template <typename CompiledFormat,
          typename... Args,
          std::enable_if_t<fmt::detail::is_compiled_string<CompiledFormat>::value, int> = 0>
inline constexpr void LOG_I(CompiledFormat &&cf, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::INFO>(std::forward<CompiledFormat>(cf),
                                                  std::forward<Args>(args)...);
}

template <typename... Args>
inline constexpr void LOG_D(fmt::format_string<Args...> &&format_str, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::DEBUG>(
            std::forward<fmt::format_string<Args...>>(format_str), std::forward<Args>(args)...);
}

template <typename CompiledFormat,
          typename... Args,
          std::enable_if_t<fmt::detail::is_compiled_string<CompiledFormat>::value, int> = 0>
inline constexpr void LOG_D(CompiledFormat &&cf, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::DEBUG>(std::forward<CompiledFormat>(cf),
                                                   std::forward<Args>(args)...);
}

template <typename... Args>
inline constexpr void LOG_T(fmt::format_string<Args...> &&format_str, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::TRACE>(
            std::forward<fmt::format_string<Args...>>(format_str), std::forward<Args>(args)...);
}

template <typename CompiledFormat,
          typename... Args,
          std::enable_if_t<fmt::detail::is_compiled_string<CompiledFormat>::value, int> = 0>
inline constexpr void LOG_T(CompiledFormat &&cf, Args &&...args) {
    Logger::getInstance().log<LOGGER_LEVEL::TRACE>(std::forward<CompiledFormat>(cf),
                                                   std::forward<Args>(args)...);
}

#endif // LOGGER_H
