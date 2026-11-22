#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE=200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#endif

#include "common/sys_utils.hpp"
#include "common/time_utils.hpp"
#include "common/string_utils.hpp"
#include <slate/log.hpp>

#ifdef SLATE_LOG_MULTI_SINK
#include <mutex>
#include <vector>
#endif
#include <stdio.h>
#include <cassert>
#include <ranges>
#include <algorithm>
#include <array>
#include <ctime>


namespace slate::log {
namespace {

constexpr struct {
    std::string_view label;
    std::string_view style;
} lut_levels[] = {
    {"[TRACE]",    "\x1b[35m"},
    {"[DEBUG]",    "\x1b[32m"},
    {"[INFO]",     "\x1b[34m"},
    {"[NOTICE]",   "\x1b[1;36m"},
    {"[WARN]",     "\x1b[1;33m"},
    {"[ERROR]",    "\x1b[1;31m"},
    {"[CRITICAL]", "\x1b[1;31;47m"},
};

//constexpr std::string_view time_style = "\x1b[1m";
constexpr std::string_view reset_style = "\x1b[0m";

// Minimum required buffer size for colored level string
constexpr int level_len_max = std::ranges::max(lut_levels
                                           | std::views::transform([](const auto& l){
                                               return l.label.size() + l.style.size() + reset_style.size();
                                           }));

[[maybe_unused]] constexpr int level_len_max_nocolor = std::ranges::max(lut_levels
                                      | std::views::transform([](const auto& l){
                                          return l.label.size();
                                      }));

[[maybe_unused]] bool is_settable_level(Level level) noexcept
{
    return level >= Level::trace && level <= Level::off;
}

// use unlocked stdio if available
#ifdef _WIN32
    #define my_putc _putc_nolock
    auto& my_fwrite = ::_fwrite_nolock;
    auto& my_flock = ::_lock_file;
    auto& my_funlock = ::_unlock_file;
#else
    #define my_putc putc_unlocked // defined by POSIX
    auto& my_flock = ::flockfile;
    auto& my_funlock = ::funlockfile;
    #if defined(__GLIBC__) || __FreeBSD__ >= 12
        auto& my_fwrite = ::fwrite_unlocked;
    #else
        auto& my_fwrite = ::fwrite;
    #endif
#endif

#ifdef SLATE_LOG_MULTI_SINK
std::vector<Sink> g_sinks{
    {.file = stdout, .level = Level::info, .color = sys::is_tty(stdout)},
};
constinit std::mutex g_write_mutex;
#else
Sink g_sink { .file=stdout, .level = Level::info, .color = sys::is_tty(stdout) };
constexpr auto const& sink = g_sink;
#endif

/**
 * Given `0 <= n < 1000`, write the 3 decimals of `n` with left-padded zeros ("%03d") to output buffer.
 * @pre 0 <= n < 1000, and [out, out + 3) is a valid range.
*/
constexpr void write_dec3(unsigned n, char* out) noexcept
{
    // A LUT mapping [0, 100) to 2 decimal digits.
    // 200 bytes in size.
    static constexpr auto lut2 = []{
        std::array<char, 100 * 2> a{};
        for (int i = 0; i < 100; ++i) {
            a[i * 2]     = char('0' + i / 10);
            a[i * 2 + 1] = char('0' + i % 10);
        }
        return a;
    }();

    assert(n < 1000);
    [[assume(n < 1000)]];

    std::copy_n(lut2.data() + 2*(n / 10), 2, out);
    out[2] = static_cast<char>('0' + n % 10);
}

constexpr auto time_maxlen = 33; // "%FT%T%z "

std::size_t sprint_iso_time(char* dst, std::size_t dst_size) noexcept
{
    // Support for time zones in std::chrono is still lacking, as of 2026.
    std::timespec ts;
    std::tm tm;
    if (std::timespec_get(&ts, TIME_UTC) == TIME_UTC && util::localtime(ts.tv_sec, tm) == 0) {
        // This is ugly!
        const auto ret = std::strftime(dst, dst_size, "%FT%T....%z ", &tm);
        if (ret > 0) {
            auto p = dst + 1 + 1 + 2 + 1 + 2      // %Y-%m-%d
                         + 1 + 2 + 1 + 2 + 1 + 2; // T%H:%M:%S
            p = std::find(p, dst + dst_size, '.');
            assert(p < dst + dst_size - 3 && p[3] == '.');
            write_dec3(ts.tv_nsec / 1'000'000, p + 1);
        }
        return ret;
    }
    return 0;
}

void write_msg_line(std::FILE* file, char* leader_start, std::size_t leader_len, std::string_view message) noexcept
{
    my_flock(file);
    my_fwrite(leader_start, leader_len, 1, file);
    my_fwrite(message.data(), message.size(), 1, file);
    my_putc('\n', file);
    my_funlock(file);
}

} // namespace

std::string_view label(Level lvl) noexcept
{
    assert(is_settable_level(lvl));
    const auto& label = lut_levels[std::to_underlying(lvl) - SLATE_LOG_LEVEL_TRACE].label;
    // Remove the brackets
    return {label.begin() + 1, label.end() - 1};
}

std::optional<Level> find_level(std::string_view name) noexcept
{
    using enum Level;
    /*--*/ if (match(name, "trace")) {
        return trace;
    } else if (match(name, "debug")) {
        return debug;
    } else if (match(name, "info")) {
        return info;
    } else if (match(name, "notice")) {
        return notice;
    } else if (match(name, "warn", "warning")) {
        return warn;
    } else if (match(name, "error", "err")) {
        return error;
    } else if (match(name, "crit", "critical")) {
        return crit;
    } else if (match(name, "off", "disable")) {
        return off;
    }
    return std::nullopt;
}

#ifdef SLATE_LOG_MULTI_SINK

std::vector<Sink>& sink_registry()
{
    return g_sinks;
}

void add_sink(Sink sink)
{
    assert(sink.file != nullptr);
    assert(is_settable_level(sink.level));
    auto& registry = sink_registry();
    const auto existing = std::ranges::find(registry, sink.file, &Sink::file);
    if (existing == registry.end()) {
        registry.push_back(sink);
    } else {
        *existing = sink;
    }
}

void remove_sink(std::FILE* file) noexcept
{
    std::erase_if(sink_registry(), [file](const Sink& sink) {
        if (sink.file == file) {
            std::fflush(file);
            return true;
        }
        return false;
    });
}

Sink* get_sink(std::FILE* file) noexcept
{
    for (auto& sink : sink_registry()) {
        if (sink.file == file) {
            return &sink;
        }
    }
    return nullptr;
}

std::span<Sink> sinks() noexcept
{
    return sink_registry();
}

bool detail::check_level(Level lvl) noexcept
{
    assert(is_log_level(lvl));
    return std::ranges::any_of(sink_registry(), [lvl](const Sink& sink) {
        return sink.level <= lvl;
    });
}

void write(Level lvl, std::string_view message)
{
    assert(is_log_level(lvl));

    const auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
    const auto& level_info = lut_levels[static_cast<int>(lvl) - SLATE_LOG_LEVEL_TRACE];
    // Formatted lines are cached.
    std::string line_plain;
    std::string line_colored;

    const std::lock_guard lock(g_write_mutex);
    for (const auto& sink : sink_registry()) {
        if (sink.level > lvl) {
            continue;
        }

        auto& line = sink.color ? line_plain : line_colored;
        if (line.empty()) {
            line = std::format("{:%T} {}{}{} {}\n", now,
                               sink.color ? level_info.style : "", level_info.label,
                               sink.color ? "\x1b[0m" : "", message);
        }
        my_fwrite(line.data(), 1, line.size(), sink.file);
    }
}

#else

Sink& get_sink() noexcept
{
    return g_sink;
}

template<bool CheckLevel>
void write(Level lvl, std::string_view message) noexcept
{
    assert(is_log_level(lvl));
    if constexpr (CheckLevel) {
        if (lvl < sink.level) {
            return;
        }
    }
    static constexpr auto bufsize = time_maxlen + level_len_max + 1; // 1 for trailing space
    char buf_leader[bufsize];
    char* p = buf_leader;
    p += sprint_iso_time(p, time_maxlen);
    const auto& lvl_info = lut_levels[std::to_underlying(lvl) - std::to_underlying(Level::trace)];
    if (sink.color) {
        p = std::ranges::copy(lvl_info.style, p).out;
    }
    p = std::ranges::copy(lvl_info.label, p).out;
    if (sink.color) {
        p = std::ranges::copy(reset_style, p).out;
    }
    // Add a trailing space
    *p++ = ' ';
    write_msg_line(sink.file, buf_leader, p - buf_leader, message);
}

template<Level Lv, bool CheckLevel>
requires IsLogLevel<Lv>
void write(std::string_view message) noexcept
{
    if constexpr (CheckLevel) {
        if (Lv < sink.level) {
            return;
        }
    }
    constexpr const auto& lvl_info = lut_levels[std::to_underlying(Lv) - std::to_underlying(Level::trace)];
    static constexpr auto bufsize = time_maxlen + lvl_info.style.size() + lvl_info.label.size() + reset_style.size()+ 1;
    char buf_leader[bufsize];
    char* p = buf_leader;
    p += sprint_iso_time(p, time_maxlen);
    if (sink.color) {
        p = std::ranges::copy(lvl_info.style, p).out;
    }
    p = std::ranges::copy(lvl_info.label, p).out;
    if (sink.color) {
        p = std::ranges::copy(reset_style, p).out;
    }
    *p++ = ' ';
    write_msg_line(sink.file, buf_leader, p - buf_leader, message);
}

template void write<true>(Level, std::string_view);
template void write<false>(Level, std::string_view);

template void write<Level::trace,  true>(std::string_view);
template void write<Level::debug,  true>(std::string_view);
template void write<Level::info,   true>(std::string_view);
template void write<Level::notice, true>(std::string_view);
template void write<Level::warn,   true>(std::string_view);
template void write<Level::error,  true>(std::string_view);
template void write<Level::crit,   true>(std::string_view);
template void write<Level::trace,  false>(std::string_view);
template void write<Level::debug,  false>(std::string_view);
template void write<Level::info,   false>(std::string_view);
template void write<Level::notice, false>(std::string_view);
template void write<Level::warn,   false>(std::string_view);
template void write<Level::error,  false>(std::string_view);
template void write<Level::crit,   false>(std::string_view);

#endif

} // namespace slate::log
