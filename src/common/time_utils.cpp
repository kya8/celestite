#include "time_utils.hpp"
#include <stdexcept>
#include <ctime>

int
util::localtime(const std::time_t& t, std::tm& tm) noexcept
{
#if defined(_WIN32)
    return ::localtime_s(&tm, &t);
#else // POSIX
    return ::localtime_r(&t, &tm) ? 0 : -1;
#endif
}

int
util::gmtime(const std::time_t& t, std::tm& tm) noexcept
{
#if defined(_WIN32)
    return ::gmtime_s(&tm, &t);
#else
    return ::gmtime_r(&t, &tm) ? 0 : -1;
#endif
}

std::string
util::format_time(const std::tm &tm, const char* fmt)
{
    for (int len = 32; len < 1024; len *= 2) {
        std::string ret;
        ret.resize_and_overwrite(len, [&](char* p, auto count) {
            return std::strftime(p, count, fmt, &tm); // returns 0 if space isn't enough
        });
        if (!ret.empty()) {
            return ret;
        }
    }
    throw std::length_error("String is too long");
}
