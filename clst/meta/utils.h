#ifndef CLST_META_UTILS_H
#define CLST_META_UTILS_H

#include <type_traits>

namespace clst::meta::utils {

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

} /* namespace utils */

#endif /* CLST_META_UTILS_H */
