#ifndef CLST_META_META_LIST_H
#define CLST_META_META_LIST_H

#include <type_traits>
#include "clst/meta/traits.h"

namespace clst::meta {

//TODO: modifiers, transforms on the list

// forwad declarations
template <auto V>
struct tag;
template <typename ...Ts>
struct type_list;
template <auto ...Vs>
struct var_list;
template <typename T, T ...Vs>
struct uni_list;


template <auto V>
struct tag {
    using value_type = decltype(V);
    static constexpr auto value = V;

    constexpr operator value_type() const noexcept {
        return V;
    }
};

template <typename ...Ts>
struct type_list {
    static constexpr auto size = sizeof...(Ts);

    template<typename F>
    static constexpr void apply(F&& f) {
        (void(f.template operator()<Ts>()), ...);
    }

    template<auto N>
    using at = traits::pack_index_type<N, Ts...>;

    template<typename F>
    static constexpr auto transform(F&& f) noexcept {
        return type_list<decltype(f(std::declval<Ts>()))...>{};
    }

    template<template<typename> typename TMP>
    using transform_template = type_holder<TMP<Ts>...>;
};

template <auto ...Vs>
struct var_list {
    static constexpr auto size = sizeof...(Vs);

    template<typename F>
    static constexpr void apply(F&& f) {
        (void(f.template operator()<Vs>()), ...); // template<auto v> operator();
    }

    template<auto N>
    static constexpr auto at = traits::pack_index_value<N, Vs...>;
    template<auto N>
    using type_at = traits::pack_index_value_type<N, Vs...>;

    using to_uni = uni_list<std::common_type_t<decltype(Vs)...>, Vs...>;
    template<auto N>
    using force_uni = uni_list<type_at<N>, Vs...>;
    using to_type_list = type_list<tag<Vs>...>;

    template<template<auto> typename TMP>
    using transform_template = var_list<TMP<Vs>::value...>;
};

namespace details {

template<typename T, T ...Vs>
struct static_store {
    static constexpr T data_[] {Vs...};
};

} /* details ns */

template <typename T, T ...Vs>
struct uni_list {
    using value_type = T;
    static constexpr auto size = sizeof...(Vs);
    // optional storage:
    //static constexpr T data_[] {Vs...};

    template<typename F>
    static constexpr void apply(F&& f) {
        (void(f.template operator()<Vs>()), ...);
    }

    template<typename F>
    static constexpr void apply2(F&& f) {
        (void(f(tag<Vs>{})), ...);  // Vs can be catched by a generic lambda
    }

    template<auto N>
    static constexpr auto at = traits::pack_index_value<N, Vs...>;

    // for this holder, we can have a runtime index-based accessor. (w/o static data_[] storage)
    // method1: instantiate a static storage class that stores Vs. (much like std::visit. But better keep this as a free function, to avoid unnecessary instantiation!)
    // method2: loop/fold through Vs, checking for the index (slower):
    static constexpr auto get(std::size_t n)
    {
        T ret{};
        decltype(n) i = 0;
        ((i++ == n? (ret = Vs, true) : false)||...); // we could replace `i++' with an integer parameter pack here...
        return ret;
    }

    template<typename F>
    static constexpr auto transform(F&& f) noexcept {  // stupid pre-C++20 hack
        return uni_list<T, decltype(f(tag<Vs>{}))::value...>{};
    }
    template<auto F>
    using transform_t = uni_list<T, F(Vs)...>;

    template<template<auto> typename TMP>
    using transform_template = uni_list<T, TMP<Vs>::value...>;

    using to_type_list = type_list<tag<Vs>...>;
    using to_var = var_list<Vs...>;

};

} /* namespace meta */

#endif /* CLST_META_META_LIST_H */
