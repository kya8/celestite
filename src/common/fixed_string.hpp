#ifndef CLST_COMMON_FIXED_STRING_HPP
#define CLST_COMMON_FIXED_STRING_HPP

#include <cstddef>
#include <string_view>

namespace clst {

// Owns a string literal so it can serve as a non-type template argument.
template<std::size_t N>
requires (N > 0)
struct FixedString {
    constexpr FixedString(const char (&literal)[N]) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) {
            chars[i] = literal[i];
        }
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept
    {
        return {chars, N - 1};
    }

    char chars[N];
};

} // namespace clst

#endif // CLST_COMMON_FIXED_STRING_HPP
