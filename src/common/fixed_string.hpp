#ifndef CLST_COMMON_FIXED_STRING_HPP
#define CLST_COMMON_FIXED_STRING_HPP

#include <charconv>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <string_view>
#include <utility>

namespace clst {

// Owns Size characters plus a trailing NUL and can be used as a template argument.
// Views and iterators borrow its storage. Writes must preserve the trailing NUL.
template<std::size_t Size>
struct FixedString {
    static_assert(Size < std::numeric_limits<std::size_t>::max());

    constexpr FixedString() noexcept = default;

    // source must provide Size readable characters. A zero-size copy may use nullptr.
    constexpr FixedString(const char* source) noexcept
    {
        if constexpr (Size != 0) {
            std::char_traits<char>::copy(chars, source, Size);
        }
    }

    [[nodiscard]] static constexpr std::size_t size() noexcept { return Size; }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Size; }

    [[nodiscard]] constexpr char* data() noexcept { return chars; }
    [[nodiscard]] constexpr const char* data() const noexcept { return chars; }
    [[nodiscard]] constexpr const char* c_str() const noexcept { return data(); }

    [[nodiscard]] constexpr char* begin() noexcept { return data(); }
    [[nodiscard]] constexpr const char* begin() const noexcept { return data(); }
    [[nodiscard]] constexpr const char* cbegin() const noexcept { return begin(); }
    [[nodiscard]] constexpr char* end() noexcept { return data() + Size; }
    [[nodiscard]] constexpr const char* end() const noexcept { return data() + Size; }
    [[nodiscard]] constexpr const char* cend() const noexcept { return end(); }

    [[nodiscard]] constexpr std::string_view view() const noexcept
    {
        return {data(), Size};
    }

    // at() throws std::out_of_range for index >= size(). operator[] requires index < size().
    [[nodiscard]] constexpr char& at(std::size_t index)
    {
        (void)view().at(index);
        return chars[index];
    }

    [[nodiscard]] constexpr const char& at(std::size_t index) const { return view().at(index); }
    [[nodiscard]] constexpr char& operator[](std::size_t index) noexcept { return chars[index]; }
    [[nodiscard]] constexpr const char& operator[](std::size_t index) const noexcept { return chars[index]; }

    // Copy the half-open range [Begin, End).
    template<std::size_t Begin, std::size_t End = Size>
    requires (Begin <= End && End <= Size)
    [[nodiscard]] constexpr auto substr() const noexcept
    {
        return FixedString<End - Begin>{data() + Begin};
    }

    template<std::size_t NewSize>
    [[nodiscard]] constexpr auto resize(char fill = '\0') const noexcept
    {
        FixedString<NewSize> result;
        constexpr auto prefix_size = NewSize < Size ? NewSize : Size;
        std::char_traits<char>::copy(result.data(), data(), prefix_size);
        std::char_traits<char>::assign(result.data() + prefix_size, NewSize - prefix_size, fill);
        return result;
    }

    [[nodiscard]] constexpr auto push_back(char value) const noexcept
    {
        return resize<Size + 1>(value);
    }

    [[nodiscard]] constexpr auto push_front(char value) const noexcept
    {
        FixedString<Size + 1> result;
        result[0] = value;
        std::char_traits<char>::copy(result.data() + 1, data(), Size);
        return result;
    }

    [[nodiscard]] constexpr auto pop_back() const noexcept requires (Size > 0)
    {
        return substr<0, Size - 1>();
    }

    [[nodiscard]] constexpr auto pop_front() const noexcept requires (Size > 0)
    {
        return substr<1>();
    }

    // first must provide Count readable elements when Count is positive.
    template<std::size_t Count, std::input_iterator Iterator>
    requires (Count < std::numeric_limits<std::size_t>::max() - Size) && std::convertible_to<std::iter_reference_t<Iterator>, char>
    [[nodiscard]] constexpr auto append(Iterator first) const
        noexcept(Count == 0 || (noexcept(std::declval<char&>() = *first) && (Count == 1 || noexcept(++first))))
    {
        FixedString<Size + Count> result;
        std::char_traits<char>::copy(result.data(), data(), Size);
        if constexpr (Count != 0) {
            char* tail = result.data() + Size;
            tail[0] = *first;
            for (std::size_t i = 1; i < Count; ++i) {
                tail[i] = *++first;
            }
        }
        return result;
    }

    char chars[Size + 1]{}; // Public storage makes FixedString a structural type.
};

template<std::size_t N>
FixedString(const char (&)[N]) -> FixedString<N - 1>;

template<std::size_t Left, std::size_t Right>
[[nodiscard]] constexpr auto operator+(const FixedString<Left>& lhs, const FixedString<Right>& rhs) noexcept
{
    return lhs.template append<Right>(rhs.begin());
}

template<std::size_t Left, std::size_t N>
[[nodiscard]] constexpr auto operator+(const FixedString<Left>& lhs, const char (&rhs)[N]) noexcept
{
    return lhs.template append<N - 1>(rhs);
}

template<std::size_t N, std::size_t Right>
[[nodiscard]] constexpr auto operator+(const char (&lhs)[N], const FixedString<Right>& rhs) noexcept
{
    return FixedString{lhs} + rhs;
}

template<auto Value>
requires std::integral<decltype(Value)> && (!std::same_as<decltype(Value), bool>)
[[nodiscard]] consteval auto to_fixed_string() noexcept
{
    constexpr auto decimal = [] {
        // digits10 + 1 accommodates the magnitude. One more character holds the sign.
        FixedString<std::numeric_limits<decltype(Value)>::digits10 + 2> buffer;
        const auto result = std::to_chars(buffer.begin(), buffer.end(), Value);
        return std::pair{buffer, static_cast<std::size_t>(result.ptr - buffer.begin())};
    }();
    return FixedString<decimal.second>{decimal.first.data()};
}

} // namespace clst

#endif // CLST_COMMON_FIXED_STRING_HPP
