#ifndef CLST_STRINGS_FIXED_STRING_H
#define CLST_STRINGS_FIXED_STRING_H

#include <cstddef>
#include <stdexcept>
#include "clst/meta/utils.h"

namespace clst::strings {

template<typename CharT, std::size_t N> // We can change this to "auto N" if we want...
class basic_fixed_string {
public:
    using type       = basic_fixed_string<CharT, N>;
    using size_type  = decltype(N);
    using value_type = CharT;
    static constexpr auto size = N;

    constexpr basic_fixed_string() noexcept = default;

    constexpr basic_fixed_string(const CharT (&s)[N+1]) noexcept { // Cannot initialize data_[] using unpacked s[i], without introducing another impl class.
        for(size_type i = 0; i < N; ++i) { // Replace with compile-time unrolled loop?
            data_[i] = s[i];
        }
    }

    constexpr auto data() const noexcept { return &data_[0]; }
    constexpr auto data() noexcept       { return &data_[0]; }

    template<size_type I>
    constexpr const auto& at() const noexcept
    {
        static_assert(I < N+1, "Subscript out of bound");
        return data_[I];
    }
    template<size_type I>
    constexpr auto& at() noexcept
    {
        return meta::utils::as_mutable(std::as_const(*this).template at<I>());
    }
    constexpr const auto& at(size_type i) const
    {
        if(i < N+1)
            return data_[i];
        else
            throw std::out_of_range("fixed_string subscript out of bound");
    }
    constexpr auto& at(size_type i)
    {
        return meta::utils::as_mutable(std::as_const(*this).at(i));
    }
    // un-checked subscript access
    constexpr const auto& operator[](size_type i) const noexcept { return data_[i]; }
    constexpr auto&       operator[](size_type i) noexcept       { return data_[i]; }


private:
    CharT data_[N+1]{0};
};
// deduction guide
template<typename CharT, auto N>
basic_fixed_string(const CharT (&)[N]) -> basic_fixed_string<CharT, N-1>;

template <typename CharT, auto N1, auto N2>
constexpr auto operator+(const basic_fixed_string<CharT, N1>& s1, const basic_fixed_string<CharT, N2>& s2) noexcept
{
    basic_fixed_string<CharT, N1 + N2> result{};
    for (decltype(N1) i = 0; i < N1; ++i) {
        result[i] = s1[i];
    }
    for (decltype(N2) j = 0; j < N2; ++j) {
        result[N1 + j] = s2[j];
    }
    return result;
}

template <typename CharT, auto N1, auto N2>
constexpr bool operator==(const basic_fixed_string<CharT, N1>& s1, const basic_fixed_string<CharT, N2>& s2) noexcept
{
    if constexpr (N1 != N2) {
        return false;
    }
    else {
        for(decltype(N1) i=0; i<N1; ++i) {
            if (s1[i] != s2[i])
                return false;
        }
        return true;
    }
}

template<std::size_t N>
using fixed_string = basic_fixed_string<char, N>;
// alias template deduction isn't available pre C++20...

// Helper function: make fixed string
template<typename CharT, auto N>
constexpr auto mkfs(const CharT (&s)[N]) noexcept
{
    basic_fixed_string<CharT, N - 1> ret{};
    for(decltype(N) i = 0; i < N; ++i) {
       ret[i] = s[i];
    }
    return ret;
}

// Get a constexpr fixed_string representing an unsigned integer.
template <unsigned long long Num>
constexpr auto digits_string() noexcept
{
    if constexpr (Num < 10)
        return fixed_string<1>({Num + '0', 0});
    else
        return digits_string<Num / 10>() + digits_string<Num % 10>();
}

//TODO: C++20 string UDL operator template
//TODO: More methods

} /* namespace strings */


#endif /* CLST_STRINGS_FIXED_STRING_H */
