#ifndef SLATE_COMMON_RANGE_UTILS_HPP
#define SLATE_COMMON_RANGE_UTILS_HPP

#include <ranges>
#include <array>

/**
 * Make a fixed std::array from a range at compile time.
 * @tparam MakeRange A callable that returns a input range.
 *         It must always return the same finite sequence.
 * @remark This is somewhat similar to `define_static_array` from C++26,
 *         however the latter needs reflection or compiler support.
 *
 * Example usage:
 * @code
static constexpr auto arr = range_to_array<
    []{
        std::string s = "a1b2c3";
        s += "def";
        return std::move(s) | std::views::filter([](char c){ return c >= 'a' && c <= 'z'; });
    }
>();
static_assert(std::string_view(arr) == "abcdef");
 * @endcode
 */
template<auto MakeRange>
    requires std::ranges::input_range<decltype(MakeRange())>
consteval auto range_to_array()
{
    auto&& range = MakeRange();
    auto it = std::ranges::begin(range);

    const auto read = [&](auto i) {
        if (i != 0) {
            ++it;
        }
        return auto(*it);
    };
    // We have to expand all elements during initialization, since the result type
    // might not be default-initializable or copyable/movable.
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return std::array{read(I)...};
    }(std::make_index_sequence<std::ranges::distance(MakeRange())>{});
}

#endif /* SLATE_COMMON_RANGE_UTILS_HPP */
