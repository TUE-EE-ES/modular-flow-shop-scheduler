#ifndef FMS_UTILS_LOGGER_HPP
#define FMS_UTILS_LOGGER_HPP

#include <fmt/compile.h>
#include <string_view>

namespace fms::utils {

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

LOGGER_LEVEL &increaseVerbosity(LOGGER_LEVEL &l);

class Logger {
public:
    static void setVerbosity(LOGGER_LEVEL l);

    static LOGGER_LEVEL getVerbosity();

    static Logger &getInstance();

    void log(LOGGER_LEVEL l, std::string_view msg);
    void logWithColor(LOGGER_LEVEL l, std::string_view msg);

    template <LOGGER_LEVEL l> void log(std::string_view msg) {
        if (l <= level) {
            logWithColor(l, msg);
        }
    }

    template <typename... Args>
    void log(LOGGER_LEVEL l, fmt::format_string<Args...> &&format_str, Args &&...args) {
        if (l <= level) {
            logWithColor(l,
                         fmt::format(std::forward<fmt::format_string<Args...>>(format_str),
                                     std::forward<Args>(args)...));
        }
    }

    template <LOGGER_LEVEL l, typename... Args>
    void log(fmt::format_string<Args...> &&format_str, Args &&...args) {
        if (l <= level) {
            logWithColor(l,
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
            logWithColor(
                    l, fmt::format(std::forward<CompiledFormat>(cf), std::forward<Args>(args)...));
        }
    }

    Logger(const Logger &) = delete;
    Logger(Logger &&) = default;
    void operator=(const Logger &) = delete;
    Logger &operator=(Logger &&) = default;

    [[nodiscard]] static inline LOGGER_LEVEL getLevel() { return getInstance().level; }

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

inline void LOG_C(std::string_view msg) { Logger::getInstance().log<LOGGER_LEVEL::CRITICAL>(msg); }

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

inline void LOG_E(std::string_view msg) { Logger::getInstance().log<LOGGER_LEVEL::ERROR>(msg); }

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

inline void LOG_W(std::string_view msg) { Logger::getInstance().log<LOGGER_LEVEL::WARNING>(msg); }

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

inline void LOG_I(std::string_view msg) { Logger::getInstance().log<LOGGER_LEVEL::INFO>(msg); }

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

inline void LOG_D(std::string_view msg) { Logger::getInstance().log<LOGGER_LEVEL::DEBUG>(msg); }

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

constexpr bool IS_LOG_C(LOGGER_LEVEL l) { return l >= LOGGER_LEVEL::CRITICAL; }
inline bool IS_LOG_C() { return IS_LOG_C(Logger::getLevel()); }

constexpr bool IS_LOG_E(LOGGER_LEVEL l) { return l >= LOGGER_LEVEL::ERROR; }
inline bool IS_LOG_E() { return IS_LOG_E(Logger::getLevel()); }

constexpr bool IS_LOG_W(LOGGER_LEVEL l) { return l >= LOGGER_LEVEL::WARNING; }
inline bool IS_LOG_W() { return IS_LOG_W(Logger::getLevel()); }

constexpr bool IS_LOG_I(LOGGER_LEVEL l) { return l >= LOGGER_LEVEL::INFO; }
inline bool IS_LOG_I() { return IS_LOG_I(Logger::getLevel()); }

constexpr bool IS_LOG_D(LOGGER_LEVEL l) { return l >= LOGGER_LEVEL::DEBUG; }
inline bool IS_LOG_D() { return IS_LOG_D(Logger::getLevel()); }

constexpr bool IS_LOG_T(LOGGER_LEVEL l) { return l >= LOGGER_LEVEL::TRACE; }
inline bool IS_LOG_T() { return IS_LOG_T(Logger::getLevel()); }

} // namespace fms::utils

namespace fms {
using utils::IS_LOG_C;
using utils::IS_LOG_D;
using utils::IS_LOG_E;
using utils::IS_LOG_I;
using utils::IS_LOG_T;
using utils::IS_LOG_W;
using utils::LOG;
using utils::LOG_C;
using utils::LOG_D;
using utils::LOG_E;
using utils::LOG_I;
using utils::LOG_T;
using utils::LOG_W;
} // namespace fms

template <> struct fmt::formatter<fms::utils::LOGGER_LEVEL> : formatter<std::string_view> {
    using BaseFormatter = formatter<std::string_view>;
    template <typename FormatContext>
    auto format(const fms::utils::LOGGER_LEVEL level, FormatContext &ctx) -> decltype(ctx.out()) {
        switch (level) {
        case fms::utils::LOGGER_LEVEL::CRITICAL:
            return BaseFormatter::format("CRITICAL", ctx);
        case fms::utils::LOGGER_LEVEL::ERROR:
            return BaseFormatter::format("ERROR", ctx);
        case fms::utils::LOGGER_LEVEL::WARNING:
            return BaseFormatter::format("WARNING", ctx);
        case fms::utils::LOGGER_LEVEL::INFO:
            return BaseFormatter::format("INFO", ctx);
        case fms::utils::LOGGER_LEVEL::DEBUG:
            return BaseFormatter::format("DEBUG", ctx);
        case fms::utils::LOGGER_LEVEL::TRACE:
            return BaseFormatter::format("TRACE", ctx);
        }
        return ctx.out();
    }
};

#endif // FMS_UTILS_LOGGER_HPP
