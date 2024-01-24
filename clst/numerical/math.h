#ifndef CLST_NUMERICAL_MATH_H
#define CLST_NUMERICAL_MATH_H

namespace clst::math
{

template<typename T>
constexpr int sgn(const T x) {
    return (x > T(0)) - (x < T(0));
}


template <unsigned long p, typename T>
constexpr auto intpow(const T x)
{
    if constexpr (p == 0)
        return T(1);
    if constexpr (p == 1)
        return x;

    const auto tmp = intpow<p / 2>(x);
    if constexpr ((p % 2) == 0)
        return tmp * tmp;
    else
        return x * tmp * tmp;
}

} // namespace clst::math


#endif /* CLST_NUMERICAL_MATH_H */
