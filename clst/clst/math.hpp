#ifndef CLST_MATH_HPP
#define CLST_MATH_HPP

namespace clst::math {

template<typename T>
constexpr int sgn(const T x) {
    return (x > T(0)) - (x < T(0));
}


template <int p, typename T>
constexpr auto int_pow(const T x)
{
    static_assert(p >= 0);
    if constexpr (p == 0)
        return T(1);
    if constexpr (p == 1)
        return x;

    const auto s = int_pow<p / 2>(x);
    if constexpr ((p % 2) == 0)
        return s * s;
    else
        return x * s * s;
}

} // namespace clst::math

#endif // CLST_MATH_HPP
