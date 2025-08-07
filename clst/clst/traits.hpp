#ifndef CLST_TRAITS_HPP
#define CLST_TRAITS_HPP

namespace clst {

template<typename ...>
constexpr bool false_v = false;

template<typename ...>
constexpr bool true_v = true;

// SFINAE helper
template<typename ...>
struct Detect {
    using type = int;
};

template<typename ...Ts>
using detect_t = typename Detect<Ts...>::type;

// SFINAE helper
template<bool B, typename T = int>
struct enable_if {};

template<typename T>
struct enable_if<true, T> { using type = T; };

template<bool B, typename T = int>
using enable_if_t = typename enable_if<B, T>::type;

} // namespace clst

#endif // CLST_TRAITS_HPP
