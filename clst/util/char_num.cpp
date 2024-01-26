#include "char_num.h"

#include <climits>
#include <cstdlib>
#include <cerrno>

namespace clst::util
{

const char* char_num(const char* str, char& out, int base) noexcept
{
    char* str_end;
    errno = 0;
#if CHAR_MIN < 0 // For platforms where `long' is no wider than `char' (!)
    const auto res = std::strtol(str, &str_end, base);
#else
    const auto res = std::strtoul(str, &str_end, base);
#endif
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr; // this step is only necessary when `long' is no wider than `char'
    if (res > CHAR_MAX || res < CHAR_MIN) return nullptr; // necessary when `long' is wider than `char'
    out = static_cast<char>(res);
    return str_end;
}

const char* char_num(const char* str, signed char& out, int base) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtol(str, &str_end, base);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    if (res > SCHAR_MAX || res < SCHAR_MIN) return nullptr;
    out = static_cast<signed char>(res);
    return str_end;
}

const char* char_num(const char* str, short& out, int base) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtol(str, &str_end, base);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    if (res > SHRT_MAX || res < SHRT_MIN) return nullptr;
    out = static_cast<short>(res);
    return str_end;
}

const char* char_num(const char* str, int& out, int base) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtol(str, &str_end, base);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    if (res > INT_MAX || res < INT_MIN) return nullptr;
    out = static_cast<int>(res);
    return str_end;
}

const char* char_num(const char* str, long& out, int base) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtol(str, &str_end, base);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    out = res;
    return str_end;
}

const char* char_num(const char* str, long long& out, int base) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtoll(str, &str_end, base);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    out = res;
    return str_end;
}


const char* char_num(const char* str, unsigned char& out, int base) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtoul(str, &str_end, base);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    if (res > UCHAR_MAX) return nullptr;
    out = static_cast<unsigned char>(res);
    return str_end;
}

const char* char_num(const char* str, unsigned short& out, int base) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtoul(str, &str_end, base);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    if (res > USHRT_MAX) return nullptr;
    out = static_cast<unsigned short>(res);
    return str_end;
}

const char* char_num(const char* str, unsigned int& out, int base) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtoul(str, &str_end, base);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    if (res > UINT_MAX) return nullptr;
    out = static_cast<unsigned int>(res);
    return str_end;
}

const char* char_num(const char* str, unsigned long& out, int base) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtoul(str, &str_end, base);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    out = res;
    return str_end;
}

const char* char_num(const char* str, unsigned long long& out, int base) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtoull(str, &str_end, base);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    out = res;
    return str_end;
}


const char* char_num(const char* str, float& out) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtof(str, &str_end);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;  // might not be set on underflow
    out = res;
    return str_end;
}

const char* char_num(const char* str, double& out) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtod(str, &str_end);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    out = res;
    return str_end;
}

const char* char_num(const char* str, long double& out) noexcept
{
    char* str_end;
    errno = 0;
    const auto res = std::strtold(str, &str_end);
    if (str_end == str)  return nullptr;
    if (errno == ERANGE) return nullptr;
    out = res;
    return str_end;
}

} // namespace clst::util
