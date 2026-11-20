#ifndef SLATE_COMMON_STRING_UTILS_HPP
#define SLATE_COMMON_STRING_UTILS_HPP

#include <type_traits>
#include <string_view>

template<typename ...Ts, std::enable_if_t<(std::is_convertible_v<Ts, std::string_view> && ...) && (sizeof...(Ts) > 0), int> = 0>
bool match(std::string_view arg, const Ts& ...Args) noexcept
{
    return ((arg == Args) || ...);
}

#endif /* SLATE_COMMON_STRING_UTILS_HPP */
