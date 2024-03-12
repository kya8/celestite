#ifndef CLST_UTIL_UTILITIES_H
#define CLST_UTIL_UTILITIES_H

#include <type_traits>
#include <utility>

namespace clst::util {

template<typename T, typename ...Ts>
constexpr bool eqOne(const T& x, const Ts&...args)
{
    return ((x == args)||...);
}

template<typename T, typename ...Ts>
constexpr bool eqAll(const T& x, const Ts&...args)
{
    return ((x == args)&&...);
}


template<typename T>
constexpr auto& as_mut(T& t) noexcept // T can be const or non-const
{
    return const_cast<std::remove_const_t<T>&>(t);
}
// This overload catches(and disallow) any rvalue argument
template<typename T>
void as_mut(const T&&) = delete;


template <typename ...Ts>
void voidify(Ts&& ...Args)
{
    ((void)(Args),...);
}


/* Helper class, for internal use */
template <typename T1, typename T2, typename = void>
class CompressPair : T1 {
    T2 t2_;

public:
    template<typename U1 = T1, typename U2 = T2>
    CompressPair(U1&& x, U2&& y) : T1(std::forward<U1>(x)), t2_(std::forward<U2>(y)) {}

    constexpr T1& first()  {return *this; }
    constexpr T2& second() {return t2_; }
};

template <typename T1, typename T2>
class CompressPair<T1, T2, std::enable_if_t<std::is_empty_v<T2>>> : T2 {
    T1 t1_;

public:
    template<typename U1 = T1, typename U2 = T2>
    CompressPair(U1&& x, U2&& y) : T2(std::forward<U2>(y)), t1_(std::forward<U1>(x)) {}

    constexpr T1& first()  {return t1_; }
    constexpr T2& second() {return *this; }
};

} // namespace clst::util


#endif /* CLST_UTIL_UTILITIES_H */
