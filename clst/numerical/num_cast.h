#ifndef CLST_NUMERICAL_NUM_CAST_H
#define CLST_NUMERICAL_NUM_CAST_H

#include "clst/meta/traits.h" // false_v
#include <type_traits>
#include <limits>
#include "exceptions.h"

namespace clst
{

template <typename To, typename From>
constexpr To num_cast(From src)
{
    static_assert(std::is_arithmetic_v<From> && std::is_arithmetic_v<To>, "Must be integral or floating point types");

    if constexpr (std::is_integral_v<From> && std::is_integral_v<To>) {
        if constexpr (std::numeric_limits<From>::max() > std::numeric_limits<To>::max()) {
            if (src > std::numeric_limits<To>::max()) throw error::BadNumCast{};
        }
        if constexpr (std::is_signed_v<From> && std::is_unsigned_v<To>) {
            if (src < 0) throw error::BadNumCast{};
        }
        else if constexpr (std::is_signed_v<From> && std::is_signed_v<To>) {
            if constexpr (std::numeric_limits<From>::lowest() < std::numeric_limits<To>::lowest()) {
                if (src < std::numeric_limits<To>::lowest()) throw error::BadNumCast{};
            }
        }
    }
    else {
        static_assert(meta::traits::false_v<From, To>, "Unsupported types");
    }

    return static_cast<To>(src);
}

} // namespace clst


#endif /* CLST_NUMERICAL_NUM_CAST_H */
