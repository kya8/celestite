#ifndef CLST_FOURCC_HPP
#define CLST_FOURCC_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <cctype>

namespace clst {

using FourCC = std::uint32_t;

#if 0
template <std::size_t N>
constexpr FourCC
fourcc(const char(&s)[N]) noexcept
{
    static_assert(N >= 4, "FourCC must contain 4 characters");

    return std::uint32_t(s[0]) << 24 | std::uint32_t(s[1]) << 16 | std::uint32_t(s[2]) << 8 | std::uint32_t(s[3]);
}

namespace detail {

// Owning wrapper for converting a string literal
template<std::size_t N>
struct StringLiteral {
    static_assert(N > 0);
    static constexpr auto size = N - 1;
    char str[N];
    constexpr StringLiteral(const char(&s)[N]) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            str[i] = s[i];
        }
    }
};

} // namespace detail

template<detail::StringLiteral Str>
constexpr std::uint32_t operator""_fc() noexcept {
    static_assert(Str.size == 4, "FourCC must contain exactly 4 characters");
    return static_cast<std::uint32_t>(Str.str[0]) << 24 |
           static_cast<std::uint32_t>(Str.str[1]) << 16 |
           static_cast<std::uint32_t>(Str.str[2]) << 8 |
           static_cast<std::uint32_t>(Str.str[3]);
}
#endif

inline namespace fourcc_literal {

consteval FourCC operator""_fc(const char* s, std::size_t len)
{
    if (len != 4) {
        throw "FourCC must contain exactly 4 characters";
    }
    return static_cast<std::uint32_t>(s[0]) << 24 |
           static_cast<std::uint32_t>(s[1]) << 16 |
           static_cast<std::uint32_t>(s[2]) << 8  |
           static_cast<std::uint32_t>(s[3]);
}

} // namespace fourcc_literal

/**
 * Convert a u32 FOURCC tag to a string.
 * @tparam Inv If true, the string begins with the LSB. Otherwise it begins with the MSB.
 * @tparam Printable If true, non-printable characters are substituted with '?'.
 */
template<bool Inv = true, bool Printable = true>
constexpr std::string // std::fixed_string?
fourcc_str(std::uint32_t u) noexcept
{
    // Printable chars in "C" locale.
    // https://en.cppreference.com/cpp/string/byte/isprint
    // static constexpr std::string_view printable = "0123456789"
    //                                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    //                                               "abcdefghijklmnopqrstuvwxyz"
    //                                               "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ ";
    char s[4]{
        char(u >> 24 & 0xffu), // MSB first
        char(u >> 16 & 0xffu),
        char(u >> 8  & 0xffu),
        char(u       & 0xffu)
    };
    if constexpr (Printable) {
        for (char& c : s) {
            if (!std::isprint(static_cast<unsigned char>(c))) {
                c = '?';
            }
        }
    }
    if constexpr (Inv) {
        return {s[3], s[2], s[1], s[0]};
    } else {
        return {s[0], s[1], s[2], s[3]};
    }
}

} // namespace clst


#endif /* CLST_FOURCC_HPP */
