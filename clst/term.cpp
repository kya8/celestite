#include "clst/term.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h> // STDOUT_FILENO
#endif
#include "clst/error.hpp"

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

} // namespace clst::term
