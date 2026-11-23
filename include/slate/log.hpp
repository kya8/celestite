#ifndef SLATE_LOG_HPP
#define SLATE_LOG_HPP

#include "slate/export.h"

#include <cstdio>
#include <format>
#include <string_view>
#include <utility>
#include <concepts>
#include <optional>
#include <algorithm>
#ifdef SLATE_LOG_MULTI_SINK
#error "Not implemented yet."
#include <span>
#endif

#define SLATE_LOG_LEVEL_TRACE    0
#define SLATE_LOG_LEVEL_DEBUG    1
#define SLATE_LOG_LEVEL_INFO     2
#define SLATE_LOG_LEVEL_NOTICE   3
#define SLATE_LOG_LEVEL_WARN     4
#define SLATE_LOG_LEVEL_ERROR    5
#define SLATE_LOG_LEVEL_CRIT     6
#define SLATE_LOG_LEVEL_OFF      7

// Synonymous levels
#define SLATE_LOG_LEVEL_MIN SLATE_LOG_LEVEL_TRACE
#define SLATE_LOG_LEVEL_MAX SLATE_LOG_LEVEL_CRIT

// Set this in compile definitions to strip lower-level log macros.
// This only applies to `SLATE_LOG_*` macros!
#ifndef SLATE_LOG_LEVEL
#define SLATE_LOG_LEVEL SLATE_LOG_LEVEL_DEBUG
#endif

#if SLATE_LOG_LEVEL < SLATE_LOG_LEVEL_MIN || SLATE_LOG_LEVEL > SLATE_LOG_LEVEL_OFF
#error "SLATE_LOG_LEVEL must be a SLATE_LOG_LEVEL_* value"
#endif

namespace slate::log {

enum class Level : int {
    trace  = SLATE_LOG_LEVEL_TRACE,
    debug  = SLATE_LOG_LEVEL_DEBUG,
    info   = SLATE_LOG_LEVEL_INFO,
    notice = SLATE_LOG_LEVEL_NOTICE,
    warn   = SLATE_LOG_LEVEL_WARN,
    error  = SLATE_LOG_LEVEL_ERROR,
    crit   = SLATE_LOG_LEVEL_CRIT,
    off    = SLATE_LOG_LEVEL_OFF,  // Settable (to turn off a sink), but should not be used in log calls.
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
SLATE_EXPORT std::string_view label(Level lvl) noexcept;

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
SLATE_EXPORT std::optional<Level> find_level(std::string_view name) noexcept;

struct Sink {
    std::FILE* file = nullptr; // Borrowed file stream open for output.
                               // The stream can still be used outside of the logger.
    Level level = Level::info;
    bool color = false;        // Colorize log output with ANSI terminal sequences.
};

// The process starts with a sink to stdout at Level::info and color enabled if stdout is a terminal.

// Callers must synchronize sink changes with all other logger calls and access
// through a obtained reference. This applies to edits through the span/pointer too.

#ifdef SLATE_LOG_MULTI_SINK

/**
 * Add a sink to the logger.
 * @pre sink.file != nullptr && (is_log_level(sink.level) || sink.level == Level::off)
 */
SLATE_EXPORT void add_sink(Sink sink);

// Remove a sink by its FILE* handle.
// Call before closing the stream.
SLATE_EXPORT void remove_sink(std::FILE* file) noexcept;

// Get a nullable, mutable reference to an existing sink, by comparing `file`.
[[nodiscard]] SLATE_EXPORT Sink* get_sink(std::FILE* file) noexcept;

// Returns a mutable view of the registered sinks. Reacquire it after add_sink or
// remove_sink. Each registered stream must be non-null, open for output, and unique.
[[nodiscard]] SLATE_EXPORT std::span<Sink> sinks() noexcept;

namespace detail {

/**
 * True if at least 1 of the sinks accept log level `lvl`.
 * @pre is_log_level(lvl)
 */
[[nodiscard]] SLATE_EXPORT bool check_level(Level lvl) noexcept;

} // namespace detail

#else

/**
 * Get a mutable reference to the global sink.
 */
SLATE_EXPORT [[nodiscard]] Sink& get_sink() noexcept;

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
SLATE_EXPORT void write(Level lvl, std::string_view message) noexcept;

template<Level Lv, bool CheckLevel = true>
requires (is_log_level(Lv))
SLATE_EXPORT void write(std::string_view message) noexcept;

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

} // namespace slate::log

#ifdef SLATE_LOG_MULTI_SINK
// Checks the runtime threshold before evaluating arguments.
// Consuming code should be careful with depending on the side effects of argument evaluation,
// as they can be ignored if the runtime log level forbids `LVL`.
#define SLATE_DETAIL_LOG(LVL, ...)                                         \
    do {                                                                   \
        if (::slate::log::detail::check_level(::slate::log::Level::LVL)) { \
            ::slate::log::write<::slate::log::Level::LVL>(__VA_ARGS__);    \
        }                                                                  \
    } while (0)
#else
// If the requested log level is rejected by sink, arguments will not be evaluated.
#define SLATE_DETAIL_LOG(LVL, ...)                                             \
    do {                                                                       \
        if (::slate::log::detail::check_level(::slate::log::Level::LVL)) {     \
            ::slate::log::write<false>(::slate::log::Level::LVL, __VA_ARGS__); \
        }                                                                      \
    } while (0)
#endif

#if SLATE_LOG_LEVEL <= SLATE_LOG_LEVEL_TRACE
#define SLATE_LOG_TRACE(...) SLATE_DETAIL_LOG(trace, __VA_ARGS__)
#else
#define SLATE_LOG_TRACE(...) ((void)0)
#endif

#if SLATE_LOG_LEVEL <= SLATE_LOG_LEVEL_DEBUG
#define SLATE_LOG_DEBUG(...) SLATE_DETAIL_LOG(debug, __VA_ARGS__)
#else
#define SLATE_LOG_DEBUG(...) ((void)0)
#endif

#if SLATE_LOG_LEVEL <= SLATE_LOG_LEVEL_INFO
#define SLATE_LOG_INFO(...) SLATE_DETAIL_LOG(info, __VA_ARGS__)
#else
#define SLATE_LOG_INFO(...) ((void)0)
#endif

#if SLATE_LOG_LEVEL <= SLATE_LOG_LEVEL_NOTICE
#define SLATE_LOG_NOTICE(...) SLATE_DETAIL_LOG(notice, __VA_ARGS__)
#else
#define SLATE_LOG_NOTICE(...) ((void)0)
#endif

#if SLATE_LOG_LEVEL <= SLATE_LOG_LEVEL_WARN
#define SLATE_LOG_WARN(...) SLATE_DETAIL_LOG(warn, __VA_ARGS__)
#else
#define SLATE_LOG_WARN(...) ((void)0)
#endif

#if SLATE_LOG_LEVEL <= SLATE_LOG_LEVEL_ERROR
#define SLATE_LOG_ERROR(...) SLATE_DETAIL_LOG(error, __VA_ARGS__)
#else
#define SLATE_LOG_ERROR(...) ((void)0)
#endif

#if SLATE_LOG_LEVEL <= SLATE_LOG_LEVEL_CRIT
#define SLATE_LOG_CRIT(...) SLATE_DETAIL_LOG(crit, __VA_ARGS__)
#else
#define SLATE_LOG_CRIT(...) ((void)0)
#endif

#endif /* SLATE_LOG_HPP */
