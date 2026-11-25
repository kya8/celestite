#ifndef CLST_LOG_HPP
#define CLST_LOG_HPP

#include "clst/export.h"

#include <cstdio>
#include <format>
#include <string_view>
#include <utility>
#include <concepts>
#include <optional>
#include <algorithm>
#ifdef CLST_LOG_MULTI_SINK
#error "Not implemented yet."
#include <span>
#endif

#define CLST_LOG_LEVEL_TRACE    0
#define CLST_LOG_LEVEL_DEBUG    1
#define CLST_LOG_LEVEL_INFO     2
#define CLST_LOG_LEVEL_NOTICE   3
#define CLST_LOG_LEVEL_WARN     4
#define CLST_LOG_LEVEL_ERROR    5
#define CLST_LOG_LEVEL_CRIT     6
#define CLST_LOG_LEVEL_OFF      7

// Synonymous levels
#define CLST_LOG_LEVEL_MIN CLST_LOG_LEVEL_TRACE
#define CLST_LOG_LEVEL_MAX CLST_LOG_LEVEL_CRIT

// Set this in compile definitions to strip lower-level log macros.
// This only applies to `CLST_LOG_*` macros!
#ifndef CLST_LOG_LEVEL
#define CLST_LOG_LEVEL CLST_LOG_LEVEL_DEBUG
#endif

#if CLST_LOG_LEVEL < CLST_LOG_LEVEL_MIN || CLST_LOG_LEVEL > CLST_LOG_LEVEL_OFF
#error "CLST_LOG_LEVEL must be a CLST_LOG_LEVEL_* value"
#endif

namespace clst::log {

enum class Level : int {
    trace  = CLST_LOG_LEVEL_TRACE,
    debug  = CLST_LOG_LEVEL_DEBUG,
    info   = CLST_LOG_LEVEL_INFO,
    notice = CLST_LOG_LEVEL_NOTICE,
    warn   = CLST_LOG_LEVEL_WARN,
    error  = CLST_LOG_LEVEL_ERROR,
    crit   = CLST_LOG_LEVEL_CRIT,
    off    = CLST_LOG_LEVEL_OFF,  // Settable (to turn off a sink), but should not be used in log calls.
    min    = trace, // Synonymous value
    max    = crit,  // Synonymous value
};

constexpr Level clamp(Level lv, bool loggable = true) noexcept {
    return std::clamp(lv, Level::min, loggable? Level::max : Level::off);
}

/**
 * Get a text label of the specified log level
 * @pre `lvl` must be one of the defined log levels, including `off'.
 */
CLST_EXPORT std::string_view label(Level lvl) noexcept;

inline constexpr bool is_settable_level(Level lv) noexcept {
    return lv >= Level::min && lv <= Level::off;
}

inline constexpr bool is_log_level(Level lv) noexcept {
    return lv >= Level::min && lv <= Level::max;
}

/**
 * Maps integer to Level
 */
inline constexpr Level to_level(int ilv) noexcept {
    return clamp(static_cast<Level>(ilv), false);
}
template<int ILv>
consteval Level to_level() {
    constexpr auto lv = static_cast<Level>(ILv);
    if (!is_settable_level(lv)) {
        throw "Not a valid level";
    }
    return lv;
}

/**
 * Find a log level according to its name.
 * @param[in] name Lower-case name of a log level.
 */
CLST_EXPORT std::optional<Level> find_level(std::string_view name) noexcept;

struct Sink {
    std::FILE* file = nullptr; // Borrowed file stream open for output.
                               // The stream can still be used outside of the logger.
    Level level = Level::info;
    bool color = false;        // Colorize log output with ANSI terminal sequences.
};

// The process starts with a sink to stdout at Level::info and color enabled if stdout is a terminal.

// Callers must synchronize sink changes with all other logger calls and access
// through a obtained reference. This applies to edits through the span/pointer too.

#ifdef CLST_LOG_MULTI_SINK

/**
 * Add a sink to the logger.
 * @pre sink.file != nullptr && (is_log_level(sink.level) || sink.level == Level::off)
 */
CLST_EXPORT void add_sink(Sink sink);

// Remove a sink by its FILE* handle.
// Call before closing the stream.
CLST_EXPORT void remove_sink(std::FILE* file) noexcept;

// Get a nullable, mutable reference to an existing sink, by comparing `file`.
[[nodiscard]] CLST_EXPORT Sink* get_sink(std::FILE* file) noexcept;

// Returns a mutable view of the registered sinks. Reacquire it after add_sink or
// remove_sink. Each registered stream must be non-null, open for output, and unique.
[[nodiscard]] CLST_EXPORT std::span<Sink> sinks() noexcept;

namespace detail {

/**
 * True if at least 1 of the sinks accept log level `lvl`.
 * @pre is_log_level(lvl)
 */
[[nodiscard]] CLST_EXPORT bool check_level(Level lvl) noexcept;

} // namespace detail

#else

/**
 * Get a mutable reference to the global sink.
 */
CLST_EXPORT [[nodiscard]] Sink& get_sink() noexcept;

namespace detail {

[[nodiscard]] inline bool check_level(Level lvl) noexcept {
    return lvl >= get_sink().level;
}

} // namespace detail

#endif

/**
 * Writes a decorated message and a newline to sink(s) that currently accept the level.
 * The logger serializes concurrent writes.
 * @pre is_log_level(lvl)
 * @tparam CheckLevel Whether to check for current sink level.
           Note this is meant for internal use.
 */
template<bool CheckLevel = true>
CLST_EXPORT void write(Level lvl, std::string_view message) noexcept;

template<Level Lv, bool CheckLevel = true>
requires (is_log_level(Lv))
CLST_EXPORT void write(std::string_view message) noexcept;

template<bool CheckLevel = true, std::formattable<char> ...Args>
requires (sizeof...(Args) > 0)
void write(Level lvl, std::format_string<Args...> fmt, Args&& ...args) {
    write<CheckLevel>(lvl, std::format(fmt, std::forward<Args>(args)...));
}

template<auto Lv, bool CheckLevel = true, std::formattable<char> ...Args>
// The `auto Lv` is here to workaround a MSVC bug where it erroneously chooses this overload.
requires (std::same_as<decltype(Lv), Level> && is_log_level(Lv) && sizeof...(Args) > 0)
void write(std::format_string<Args...> fmt, Args&& ...args) {
    write<Lv, CheckLevel>(std::format(fmt, std::forward<Args>(args)...));
}

} // namespace clst::log

#ifdef CLST_LOG_MULTI_SINK
// Checks the runtime threshold before evaluating arguments.
// Consuming code should be careful with depending on the side effects of argument evaluation,
// as they can be ignored if the runtime log level forbids `LVL`.
#define CLST_DETAIL_LOG(LVL, ...)                                         \
    do {                                                                   \
        if (::clst::log::detail::check_level(::clst::log::Level::LVL)) { \
            ::clst::log::write<::clst::log::Level::LVL>(__VA_ARGS__);    \
        }                                                                  \
    } while (0)
#else
// If the requested log level is rejected by sink, arguments will not be evaluated.
#define CLST_DETAIL_LOG(LVL, ...)                                             \
    do {                                                                       \
        if (::clst::log::detail::check_level(::clst::log::Level::LVL)) {     \
            ::clst::log::write<false>(::clst::log::Level::LVL, __VA_ARGS__); \
        }                                                                      \
    } while (0)
#endif

#if CLST_LOG_LEVEL <= CLST_LOG_LEVEL_TRACE
#define CLST_LOG_TRACE(...) CLST_DETAIL_LOG(trace, __VA_ARGS__)
#else
#define CLST_LOG_TRACE(...) ((void)0)
#endif

#if CLST_LOG_LEVEL <= CLST_LOG_LEVEL_DEBUG
#define CLST_LOG_DEBUG(...) CLST_DETAIL_LOG(debug, __VA_ARGS__)
#else
#define CLST_LOG_DEBUG(...) ((void)0)
#endif

#if CLST_LOG_LEVEL <= CLST_LOG_LEVEL_INFO
#define CLST_LOG_INFO(...) CLST_DETAIL_LOG(info, __VA_ARGS__)
#else
#define CLST_LOG_INFO(...) ((void)0)
#endif

#if CLST_LOG_LEVEL <= CLST_LOG_LEVEL_NOTICE
#define CLST_LOG_NOTICE(...) CLST_DETAIL_LOG(notice, __VA_ARGS__)
#else
#define CLST_LOG_NOTICE(...) ((void)0)
#endif

#if CLST_LOG_LEVEL <= CLST_LOG_LEVEL_WARN
#define CLST_LOG_WARN(...) CLST_DETAIL_LOG(warn, __VA_ARGS__)
#else
#define CLST_LOG_WARN(...) ((void)0)
#endif

#if CLST_LOG_LEVEL <= CLST_LOG_LEVEL_ERROR
#define CLST_LOG_ERROR(...) CLST_DETAIL_LOG(error, __VA_ARGS__)
#else
#define CLST_LOG_ERROR(...) ((void)0)
#endif

#if CLST_LOG_LEVEL <= CLST_LOG_LEVEL_CRIT
#define CLST_LOG_CRIT(...) CLST_DETAIL_LOG(crit, __VA_ARGS__)
#else
#define CLST_LOG_CRIT(...) ((void)0)
#endif

#endif /* CLST_LOG_HPP */
