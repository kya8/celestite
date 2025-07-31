#include "clst/term.hpp"
#include "clst/error.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h> // _isatty
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <cstdlib> // std::getenv
#include <string_view>

namespace clst::term {

TermSize get_term_size()
{
#ifdef _WIN32
    const auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) {
        SystemError::throw_last();
    }
    CONSOLE_SCREEN_BUFFER_INFOEX info;
    info.cbSize = sizeof(info);
    if (!GetConsoleScreenBufferInfoEx(handle, &info)) {
        SystemError::throw_last();
    }
    return {info.dwSize.Y, info.dwSize.X};
#else
    winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) {
        SystemError::throw_last();
    }
    return {ws.ws_row, ws.ws_col};
#endif
}

bool
is_tty(std::FILE* file) noexcept
{
#ifdef _WIN32
    return _isatty(_fileno(file));
#else
    return isatty(fileno(file));
#endif
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // std::getenv
#endif
bool is_colorterm() noexcept
{
    const auto to_sv = [](const char* s) { // Since constructing string_view from null char* is UB.
        return s? std::string_view{s} : std::string_view{};
    };
    const std::string_view term = to_sv(std::getenv("TERM"));
    const std::string_view colorterm = to_sv(std::getenv("COLORTERM"));
    const auto contains = [&](std::string_view s) {
        return term.find(s) != std::string_view::npos || colorterm.find(s) != std::string_view::npos;
    };

    return colorterm == "1" ||
    // FIXME: Use GetConsoleMode on Win32
    // Ref: https://github.com/microsoft/terminal/issues/1040
           std::getenv("WT_SESSION") ||
           contains("256") ||
           contains("24bit") ||
           contains("truecolor");
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

bool is_colorterm(std::FILE* file) noexcept
{
    return is_tty(file) && is_colorterm();
}

} // namespace clst::term
