#ifndef SLATE_COMMON_NUM_CAST_HPP
#define SLATE_COMMON_NUM_CAST_HPP

#include <type_traits>
#include <limits>
#include <optional>
#include <stdexcept>
#include <concepts>

namespace util {

class num_cast_range_error : public std::range_error {
public:
    using std::range_error::range_error;
    num_cast_range_error() : std::range_error("num_cast out of range!") {}
};

// Returns optional<Tgt> if conversion is potentially lossy; otherwise returns Tgt.
template <std::integral Tgt, std::integral Src>
constexpr auto num_cast(Src src) noexcept
{
    static constexpr bool check_upper = std::numeric_limits<Src>::max() > std::numeric_limits<Tgt>::max();
    static constexpr bool check_lower = static_cast<std::make_signed_t<Src>>(std::numeric_limits<Src>::lowest()) < static_cast<std::make_signed_t<Tgt>>(std::numeric_limits<Tgt>::lowest()); // prevent 0u < -1 errors
    if constexpr (check_upper || check_lower) {
        if constexpr (check_upper) {
            if (src > std::numeric_limits<Tgt>::max()) {
                return std::optional<Tgt>{};
            }
        }
        if constexpr (check_lower) {
            if (src < static_cast<std::make_signed_t<Tgt>>(std::numeric_limits<Tgt>::lowest())) {
                return std::optional<Tgt>{};
            }
        }
        return std::optional<Tgt>{static_cast<Tgt>(src)};
    } else {
        return static_cast<Tgt>(src);
    }
}

template<typename Src, typename Tgt>
constexpr bool is_lossless_num_cast = requires(Src src){ {num_cast<Tgt>(src)} -> std::same_as<Tgt>; };

template <typename Tgt, typename Src>
constexpr Tgt num_cast_ex(Src src) noexcept(is_lossless_num_cast<Src, Tgt>)
{
    const auto result = num_cast<Tgt>(src);
    if constexpr (is_lossless_num_cast<Src, Tgt>) {
        return result;
    } else if (result.has_value()) {
        return *result;
    }
    throw num_cast_range_error{};
}

} // namespace util

#endif // SLATE_COMMON_NUM_CAST_HPP
