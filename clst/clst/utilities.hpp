#ifndef CLST_UTILITIES_HPP
#define CLST_UTILITIES_HPP

#include <type_traits>
#include <utility>

namespace clst {

template<typename T, typename ...Ts>
constexpr bool eq_one(const T& x, const Ts&...args)
{
    return ((x == args)||...);
}

template<typename T, typename ...Ts>
constexpr bool eq_all(const T& x, const Ts&...args)
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


/* Helper EBO class */
template <typename T1, typename T2, typename = void>
class CompressPair : protected T1 {
    T2 t2_;

public:
    template<typename U1 = T1, typename U2 = T2>
    constexpr CompressPair(U1&& x, U2&& y) : T1(std::forward<U1>(x)), t2_(std::forward<U2>(y)) {}

    constexpr T1& first() noexcept        { return *this; }
    constexpr const T1& first() const noexcept  { return *this; }
    constexpr auto& second() noexcept       { return t2_; }
    constexpr auto& second() const noexcept { return t2_; }
};

template <typename T1, typename T2>
class CompressPair<T1, T2, std::enable_if_t<std::is_empty_v<T2>>> : protected T2 {
    T1 t1_;

public:
    template<typename U1 = T1, typename U2 = T2>
    constexpr CompressPair(U1&& x, U2&& y) : T2(std::forward<U2>(y)), t1_(std::forward<U1>(x)) {}

    constexpr auto& first() noexcept        { return t1_; }
    constexpr auto& first() const noexcept  { return t1_; }
    constexpr T2& second() noexcept       { return *this; }
    constexpr const T2& second() const noexcept { return *this; }
};

} // namespace clst

#endif // CLST_UTILITIES_HPP
