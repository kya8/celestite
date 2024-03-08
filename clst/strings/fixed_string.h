#ifndef CLST_STRINGS_FIXED_STRING_H
#define CLST_STRINGS_FIXED_STRING_H

#include <cstddef>           // std::size_t
#include <stdexcept>         // std::out_of_range
#include "clst/meta/utils.h" // as_mut
#include <utility>           // std::as_const

// Note:
// Compile-time typing based on the CONTENTS of string data is restricted.
// We could use char[]/char* as template parameters, however that requires static storage (and no string literals.)

namespace clst::strings {

namespace details {

template<typename CharT>
constexpr auto cstr_size(const CharT* s) {
    std::size_t i = 0;
    while (*s != 0) {
        ++i;
        ++s;
    }
    return i;
}

} // namespace details

template<typename CharT, std::size_t N> // We can change this to "auto N" if we want...
class BasicFixedString {
public:
    using type       = BasicFixedString<CharT, N>;
    using size_type  = decltype(N);
    using value_type = CharT;
    static constexpr auto size = N;

    constexpr BasicFixedString() noexcept = default;

    constexpr BasicFixedString(const CharT (&s)[N+1]) noexcept { // Cannot initialize data_[] using unpacked s[i], without introducing another impl class.
        for(size_type i = 0; i < N; ++i) { // Replace with compile-time unrolled loop?
            data_[i] = s[i];
        }
    }

    constexpr auto str() const noexcept { return &data_[0]; }
    constexpr auto str() noexcept       { return &data_[0]; }

    template<size_type I>
    constexpr const auto& at() const noexcept
    {
        static_assert(I < N+1, "Subscript out of bound");
        return data_[I];
    }
    template<size_type I>
    constexpr auto& at() noexcept
    {
        return meta::utils::as_mut(std::as_const(*this).template at<I>());
    }
    constexpr const auto& at(size_type i) const
    {
        if(i < N+1)
            return data_[i];
        else
            throw std::out_of_range("FixedString subscript out of bound");
    }
    constexpr auto& at(size_type i)
    {
        return meta::utils::as_mut(std::as_const(*this).at(i));
    }
    // un-checked subscript access
    constexpr const auto& operator[](size_type i) const noexcept { return data_[i]; }
    constexpr auto&       operator[](size_type i) noexcept       { return data_[i]; }

    // compile-time slicing
    template<size_type Begin, size_type End>
    constexpr auto slice() const noexcept {
        static_assert(End < N+1 && Begin >= 0, "Slicing out of bound");
        static_assert(End >= Begin, "Invalid slicing index");

        BasicFixedString<CharT, End - Begin> ret{};
        for(size_type i = 0; i < End-Begin; ++i) {
            ret[i] = data_[Begin + i];
        }
        return ret;
    }

    // A FixedString is ill-formed if it contains NUL bytes other than at data_[N].

    // runtime slicing, that returns a (ill-formed) FixedString with same size.
    // No checks performed!
    constexpr auto slice(size_type begin, size_type end) const noexcept {
        BasicFixedString<CharT, N> ret{};
        for(size_type i = 0; i < end-begin; ++i) {
            ret[i] = data_[begin + i];
        }
        return ret;
    }

    // Get actual length of the nul-terminated string, in contrast to the storage size.
    // Returns N+1 if storage is not nul-terminated.
    constexpr auto actualSize() const noexcept {
        size_type i = 0;
        while (i < N+1 && data_[i] != 0) {
            ++i;
        }
        return i;
    }

    constexpr const auto& data() const noexcept { return data_; }
    constexpr auto&       data() noexcept       { return data_; }

private:
    CharT data_[N+1]{0};
};
// deduction guide
template<typename CharT, auto N>
BasicFixedString(const CharT (&)[N]) -> BasicFixedString<CharT, N-1>;

template <typename CharT, auto N1, auto N2>
constexpr auto operator+(const BasicFixedString<CharT, N1>& s1, const BasicFixedString<CharT, N2>& s2) noexcept
{
    BasicFixedString<CharT, N1 + N2> result{};
    for (decltype(N1) i = 0; i < N1; ++i) {
        result[i] = s1[i];
    }
    for (decltype(N2) j = 0; j < N2; ++j) {
        result[N1 + j] = s2[j];
    }
    return result;
}

template <typename CharT, auto N1, auto N2>
constexpr bool operator==(const BasicFixedString<CharT, N1>& s1, const BasicFixedString<CharT, N2>& s2) noexcept
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
using FixedString = BasicFixedString<char, N>;
// alias template deduction isn't available pre C++20...

// Helper function: make fixed string
template<typename CharT, std::size_t N> // "auto N" triggers intellisense errors...
constexpr auto mkfs(const CharT (&s)[N]) noexcept
{
    BasicFixedString<CharT, N - 1> ret{};
    for(decltype(N) i = 0; i < N; ++i) {
       ret[i] = s[i];
    }
    return ret;
}

// Get a constexpr FixedString representing an unsigned integer.
template <unsigned long long Num>
constexpr auto digitsString() noexcept
{
    if constexpr (Num < 10)
        return FixedString<1>({Num + '0', 0});
    else
        return digitsString<Num / 10>() + digitsString<Num % 10>();
}

//TODO: C++20 string UDL operator template
//TODO: More methods

} /* namespace clst::strings */


#endif /* CLST_STRINGS_FIXED_STRING_H */
