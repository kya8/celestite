#ifndef CLST_META_TRAITS_H
#define CLST_META_TRAITS_H

#include <type_traits>
#include <utility>
#include <cstddef>

namespace clst::meta::traits {

template<typename T, typename ...Ts>
constexpr bool is_one_of_v = std::disjunction_v<std::is_same<T, Ts>...>;

template<typename T, typename ...Ts>
constexpr bool is_all_same_v = std::conjunction_v<std::is_same<T, Ts>...>;

/* helper dependent types */
template<typename...>
constexpr bool always_false_v = false;
template<typename...>
constexpr bool always_true_v = true;
template<typename T, typename ...>
using always_T_with_types = T;
template<typename T, auto ...>
using always_T_with_values = T;


template <typename T, template <typename...> typename Template>
struct is_instantiation_of : std::false_type {};
template <template <typename...> typename Template, typename... Args>
struct is_instantiation_of<Template<Args...>, Template> : std::true_type {};

template<typename>
struct is_template_instantiation : std::false_type {};

template<template<typename...> typename TMPL, typename ...Ps>
struct is_template_instantiation<TMPL<Ps...>> : std::true_type {};


namespace details {

template<typename S, typename T, typename = void>
struct is_streamable_impl : std::false_type {};

template<typename S, typename T>
struct is_streamable_impl <S, T,
                           std::void_t<decltype( std::declval<S&>() << std::declval<T>() )>>
                          : std::true_type {};

}

// For some reason, this doesn't work for some stl templates, that have user-defined operator<< AFTER including this header.
template<typename S, typename T>
constexpr bool is_streamable_v = details::is_streamable_impl<S, T>::value;


// Add types to a user-provided template instantiation, removing duplicates.
template <typename T, typename...>
struct append_unique_types {
    using type = T;
};
template <template<typename...> typename TMPL, typename ...Ts, typename U, typename ...Us>
struct append_unique_types<TMPL<Ts...>, U, Us...> : std::conditional_t<(std::disjunction_v<std::is_same<U, Ts>...>),
                                                                        append_unique_types<TMPL<Ts...>, Us...>,
                                                                        append_unique_types<TMPL<Ts..., U>, Us...>>
{};


template<std::size_t I, typename T>
struct indexed_type {
    static constexpr auto index = I;
    using type = T;
};

template<std::size_t I, auto Val>
struct indexed_value {
    static constexpr auto index = I;
    static constexpr auto value = Val;
    using value_type = decltype(Val);
};

namespace details {

// parameter pack indexing
// implemented with multiple inheritance

template<typename, typename...>
struct pack_index_impl;
template<std::size_t ...Is, typename ...Ts>
struct pack_index_impl<std::index_sequence<Is...>, Ts...> : indexed_type<Is, Ts>... {};

template<typename ...Ts>
using pack_index = pack_index_impl<std::index_sequence_for<Ts...>, Ts...>;

template<std::size_t N, typename ...Ts>
struct get_pack_index_type {
private:
    template<typename T>
    static indexed_type<N, T> test(indexed_type<N, T>);
public:
    using type = typename decltype(test(pack_index<Ts...>{}))::type;
};


template<typename, auto...>
struct pack_index_impl_val;
template<std::size_t ...Is, auto ...Vs>
struct pack_index_impl_val<std::index_sequence<Is...>, Vs...> : indexed_value<Is, Vs>... {};

template<auto ...Vs>
using pack_index_vals = pack_index_impl_val<std::make_index_sequence<sizeof...(Vs)>, Vs...>;

template<std::size_t N, auto ...Vs>
struct get_pack_index_val {
private:
    template<auto Val>
    static indexed_value<N, Val> test(indexed_value<N, Val>);
public:
    using info = decltype(test(pack_index_vals<Vs...>{}));
    using type = typename info::value_type;    // this is required, in order to preserve the original type.
    static constexpr auto value = info::value; // this value always has const type!

};

} /* namespace details */

template<std::size_t N, typename ...Ts>
using pack_index_type = typename details::get_pack_index_type<N, Ts...>::type;

template<std::size_t N, auto ...Vs>
using pack_index_value_info = typename details::get_pack_index_val<N, Vs...>::info;
template<std::size_t N, auto ...Vs>
constexpr auto pack_index_value = details::get_pack_index_val<N, Vs...>::value;
template<std::size_t N, auto ...Vs>
using pack_index_value_type = typename details::get_pack_index_val<N, Vs...>::type;


} /* namespace traits */

#endif /* CLST_META_TRAITS_H */
