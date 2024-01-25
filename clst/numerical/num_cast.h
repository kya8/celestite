#ifndef CLST_NUMERICAL_NUM_CAST_H
#define CLST_NUMERICAL_NUM_CAST_H

#include "clst/common/exceptions.h"
#include <type_traits>
#include <limits>
#include "clst/meta/traits.h"

namespace clst::numerical
{

namespace details
{

template<typename T>
constexpr auto max = std::numeric_limits<T>::max();
template<typename T>
constexpr auto min = std::numeric_limits<T>::min();
template<typename T>
constexpr auto lowest = std::numeric_limits<T>::lowest();

} // namespace details


template<typename To, typename From>
constexpr To num_cast(From src)
{
    using details::max, details::lowest;

    if constexpr (std::is_integral_v<To>) {
        if constexpr(std::is_unsigned_v<To>) {

        }
    }
    else {
        static_assert(meta::traits::always_false_v<From, To>, "Unsupported type for num_cast!"); // CWG2518
    }

    return static_cast<To>(src);
}

} // namespace clst::numerical


#endif /* CLST_NUMERICAL_NUM_CAST_H */
