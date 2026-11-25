#ifndef CLST_COMMON_STRING_UTILS_HPP
#define CLST_COMMON_STRING_UTILS_HPP

#include <type_traits>
#include <string_view>
#include <algorithm>
#include <concepts>
#include <cctype>

namespace util {

template<std::convertible_to<std::string_view> T>
constexpr decltype(auto) to_sv(const T& src) noexcept
{
    if constexpr (std::same_as<T, std::string_view>) {
        return src;
    } else {
        return std::string_view{src};
    }
}

template<typename ...Ts, std::enable_if_t<(std::is_convertible_v<Ts, std::string_view> && ...) && (sizeof...(Ts) > 0), int> = 0>
constexpr bool match_any(std::string_view ref, const Ts& ...args) noexcept
{
    return ((ref == to_sv(args)) || ...);
}

// "String case-Insensentive EQ"
template<std::convertible_to<std::string_view> ...Ts>
bool sieq(std::string_view ref, const Ts& ...args) noexcept
{
    return (std::ranges::equal(ref, to_sv(args), [](unsigned char l, unsigned char r) {
        return std::tolower(l) == std::tolower(r);
    }) && ...);
}

template<std::convertible_to<std::string_view> ...Ts>
requires (sizeof...(Ts) > 0)
constexpr bool imatch_any(std::string_view ref, const Ts& ...args) noexcept
{
    return (sieq(ref, args) || ...);
}

} // namespace util

#endif /* CLST_COMMON_STRING_UTILS_HPP */
