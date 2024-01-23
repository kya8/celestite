#ifndef CLST_NUMERICAL_MATH_H
#define CLST_NUMERICAL_MATH_H

namespace clst::math
{

template<typename T>
constexpr int sgn(const T x) {
    return (x > T(0)) - (x < T(0));
}

} // namespace clst::math


#endif /* CLST_NUMERICAL_MATH_H */
