#ifndef CLST_STRINGS_TYPE_NAME_H
#define CLST_STRINGS_TYPE_NAME_H

#include "clst/strings/fixed_string.h"
#include <initializer_list>
//TODO: make these optional
#include <vector>
#include <tuple>

namespace clst::strings
{

namespace typename_details {

template<template<typename ...> typename>
struct template_name {
    static constexpr auto value = mkfs("unknown template");
};

template<typename T>
struct type_name {
    static constexpr auto value = mkfs("unknown type");
};

#define CLST_MAKE_TYPE_NAME_SPECIALIZATION_WITH_NAME(Type, Name) \
template<>                                                  \
struct type_name<Type> {                                    \
    static constexpr auto value = mkfs(#Name);              \
};
#define CLST_MAKE_TYPE_NAME_SPECIALIZATION(Type) CLST_MAKE_TYPE_NAME_SPECIALIZATION_WITH_NAME(Type, Type)

#define CLST_MAKE_TEMPLATE_NAME_SPECIALIZATION_WITH_NAME(Template, Name) \
template<>                                                          \
struct template_name<Template> {                                    \
    static constexpr auto value = mkfs(#Name);                      \
};
#define CLST_MAKE_TEMPLATE_NAME_SPECIALIZATION(Template) CLST_MAKE_TEMPLATE_NAME_SPECIALIZATION_WITH_NAME(Template, Template)

/* distinct primitive types */
CLST_MAKE_TYPE_NAME_SPECIALIZATION(bool)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(char)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(signed char)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(unsigned char)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(wchar_t)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(char16_t)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(char32_t)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(short)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(unsigned short)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(int)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(unsigned int)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(long)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(unsigned long)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(long long)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(unsigned long long)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(float)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(double)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(long double)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(void)
CLST_MAKE_TYPE_NAME_SPECIALIZATION(std::nullptr_t)

template<typename T>
struct type_name<const T> {
    static constexpr auto value = mkfs("const ") + type_name<T>::value;
};
template<typename T>
struct type_name<volatile T> {
    static constexpr auto value = mkfs("volatile ") + type_name<T>::value;
};

template<typename T>
struct type_name<T*> {
    static constexpr auto value = mkfs("pointer to ") + type_name<T>::value;
};

template<typename T, std::size_t N>
struct type_name<T[N]> {
    static constexpr auto value = mkfs("array of ") + digits_string<N>() + mkfs(" ") + type_name<T>::value;
};

template<typename T, std::size_t N>
struct type_name<const T[N]> {  // otherwise ambiguous
    static constexpr auto value = mkfs("const array of ") + digits_string<N>() + mkfs(" ") + type_name<T>::value;
};

template<typename T>
struct type_name<T&&> {
    static constexpr auto value = mkfs("rvalue reference to ") + type_name<T>::value;
};
template<typename T>
struct type_name<T&> {
    static constexpr auto value = mkfs("lvalue reference to ") + type_name<T>::value;
};

// helper function
template<typename FirstT, typename ...Ts>
constexpr auto type_list() {
    return type_name<FirstT>::value + (mkfs("") + ... + (mkfs(", ") + type_name<Ts>::value));
}
template<typename ...Ts, std::enable_if_t<sizeof...(Ts) == 0, bool> = 1> // overload to match empty Ts
constexpr auto type_list() {
    return mkfs("");
}

template<typename R, typename ...Args>
struct type_name<R(Args...)> {
    static constexpr auto value = mkfs("function returning ") + type_name<R>::value +
                                    mkfs(" with argument (") + type_list<Args...>() + mkfs(")");
};

// templates
CLST_MAKE_TEMPLATE_NAME_SPECIALIZATION(std::initializer_list)

template<typename ...Ts>
struct type_name<std::tuple<Ts...>> {
    static constexpr auto value = mkfs("std::tuple of [") + type_list<Ts...>() + mkfs("]");
};

template<typename V, typename ...Rem>
struct type_name<std::vector<V, Rem...>> { // discard other template parameters
    static constexpr auto value = mkfs("std::vector<") + type_name<V>::value + mkfs(">");
};

template<template<typename ...> typename T, typename ...Ps>
struct type_name<T<Ps...>> {
    static constexpr auto value = template_name<T>::value +
    mkfs("<") + type_list<Ps...>() + mkfs(">");
};

#undef CLST_MAKE_TYPE_NAME_SPECIALIZATION
#undef CLST_MAKE_TYPE_NAME_SPECIALIZATION_WITH_NAME
#undef CLST_MAKE_TEMPLATE_NAME_SPECIALIZATION
#undef CLST_MAKE_TEMPLATE_NAME_SPECIALIZATION_WITH_NAME

} // namespace typename_details

template<typename T>
constexpr auto type_name_fstr = typename_details::type_name<T>::value;

template<typename T>
constexpr auto type_name_str = type_name_fstr<T>.str();

} // namespace clst::strings


#endif /* CLST_STRINGS_TYPE_NAME_H */
