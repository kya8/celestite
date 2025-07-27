#ifndef CLST_NUM_CAST_HPP
#define CLST_NUM_CAST_HPP

#include <type_traits>
#include <limits>
#include <optional>
#include <stdexcept>

namespace clst {

// Returns optional<To> if conversion is potentially lossy; otherwise returns To.
template <typename To, typename From>
constexpr auto num_cast(From src)
{
    static_assert(std::is_integral_v<From> && std::is_integral_v<To>, "Must be integral types");

    // static constexpr in constexpr func is C++23
    constexpr bool check_upper = std::numeric_limits<From>::max() > std::numeric_limits<To>::max();
    constexpr bool check_lower = static_cast<std::make_signed_t<From>>(std::numeric_limits<From>::lowest()) < static_cast<std::make_signed_t<To>>(std::numeric_limits<To>::lowest()); // prevent 0u < -1 errors
    if constexpr (check_upper || check_lower) {
        if constexpr (check_upper) {
            if (src > std::numeric_limits<To>::max())
                return std::optional<To>{};
        }
        if constexpr (check_lower) {
            if (src < static_cast<std::make_signed_t<To>>(std::numeric_limits<To>::lowest()))
                return std::optional<To>{};
        }
        return std::optional<To>{static_cast<To>(src)};
    } else {
        return static_cast<To>(src);
    }
}

template<typename From, typename To>
constexpr bool is_lossless_num_cast = std::is_same_v<decltype(num_cast<To>(std::declval<From>())), To>;

template <typename To, typename From>
constexpr To num_cast_ex(From src) noexcept(is_lossless_num_cast<From, To>)
{
    const auto to = num_cast<To>(src);
    if constexpr (is_lossless_num_cast<From, To>) {
        return to;
    } else {
        if (to.has_value())
            return *to;
        throw std::range_error("num_cast range error");
    }
}

} // namespace clst

#endif // CLST_NUM_CAST_HPP
