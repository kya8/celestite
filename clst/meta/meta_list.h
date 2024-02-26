#ifndef CLST_META_META_LIST_H
#define CLST_META_META_LIST_H

#include <type_traits>
#include "clst/meta/traits.h"

namespace clst::meta {

//TODO: modifiers, transforms on the list

// forwad declarations
template <auto V>
struct Tag;
template <typename ...Ts>
struct TypeList;
template <auto ...Vs>
struct VarList;
template <typename T, T ...Vs>
struct UniList;


template <auto V>
struct Tag {
    using value_type = decltype(V);
    static constexpr auto value = V;

    constexpr operator value_type() const noexcept {
        return V;
    }
};

template <typename ...Ts>
struct TypeList {
    static constexpr auto size = sizeof...(Ts);

    template<typename F>
    static constexpr void apply(F&& f) {
        (void(f.template operator()<Ts>()), ...);
    }

    template<auto N>
    using at = traits::pack_index_type<N, Ts...>;

    template<typename F>
    static constexpr auto transform(F&& f) noexcept {
        return TypeList<decltype(f(std::declval<Ts>()))...>{};
    }

    template<template<typename> typename TMP>
    using transform_template = type_holder<TMP<Ts>...>;
};

template <auto ...Vs>
struct VarList {
    static constexpr auto size = sizeof...(Vs);

    template<typename F>
    static constexpr void apply(F&& f) {
        (void(f.template operator()<Vs>()), ...); // template<auto v> operator();
    }

    template<auto N>
    static constexpr auto at = traits::pack_index_value<N, Vs...>;
    template<auto N>
    using type_at = traits::pack_index_value_type<N, Vs...>;

    using to_uni = UniList<std::common_type_t<decltype(Vs)...>, Vs...>;
    template<auto N>
    using force_uni = UniList<type_at<N>, Vs...>;
    using to_type_list = TypeList<Tag<Vs>...>;

    template<template<auto> typename TMP>
    using transform_template = VarList<TMP<Vs>::value...>;
};

namespace details {

template<typename T, T ...Vs>
struct static_store {
    static constexpr T data_[] {Vs...};
};

} /* details ns */

template <typename T, T ...Vs>
struct UniList {
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
        (void(f(Tag<Vs>{})), ...);  // Vs can be catched by a generic lambda
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
        return UniList<T, decltype(f(Tag<Vs>{}))::value...>{};
    }
    template<auto F>
    using transform_t = UniList<T, F(Vs)...>;

    template<template<auto> typename TMP>
    using transform_template = UniList<T, TMP<Vs>::value...>;

    using to_type_list = TypeList<Tag<Vs>...>;
    using to_var = VarList<Vs...>;

};

} /* namespace meta */

#endif /* CLST_META_META_LIST_H */
