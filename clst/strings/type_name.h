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
struct TemplateName {
    static constexpr auto value = mkfs("unknown template");
};

template<typename T>
struct TypeName {
    static constexpr auto value = mkfs("unknown type");
};

#define CLST_MAKE_TYPE_NAME_SPECIALIZATION_WITH_NAME(Type, Name) \
template<>                                                  \
struct TypeName<Type> {                                    \
    static constexpr auto value = mkfs(#Name);              \
};
#define CLST_MAKE_TYPE_NAME_SPECIALIZATION(Type) CLST_MAKE_TYPE_NAME_SPECIALIZATION_WITH_NAME(Type, Type)

#define CLST_MAKE_TEMPLATE_NAME_SPECIALIZATION_WITH_NAME(Template, Name) \
template<>                                                          \
struct TemplateName<Template> {                                    \
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
struct TypeName<const T> {
    static constexpr auto value = mkfs("const ") + TypeName<T>::value;
};
template<typename T>
struct TypeName<volatile T> {
    static constexpr auto value = mkfs("volatile ") + TypeName<T>::value;
};

template<typename T>
struct TypeName<T*> {
    static constexpr auto value = mkfs("pointer to ") + TypeName<T>::value;
};

template<typename T, std::size_t N>
struct TypeName<T[N]> {
    static constexpr auto value = mkfs("array of ") + digitsString<N>() + mkfs(" ") + TypeName<T>::value;
};

template<typename T, std::size_t N>
struct TypeName<const T[N]> {  // otherwise ambiguous
    static constexpr auto value = mkfs("const array of ") + digitsString<N>() + mkfs(" ") + TypeName<T>::value;
};

template<typename T>
struct TypeName<T&&> {
    static constexpr auto value = mkfs("rvalue reference to ") + TypeName<T>::value;
};
template<typename T>
struct TypeName<T&> {
    static constexpr auto value = mkfs("lvalue reference to ") + TypeName<T>::value;
};

// helper function
template<typename FirstT, typename ...Ts>
constexpr auto typeList() {
    return TypeName<FirstT>::value + (mkfs("") + ... + (mkfs(", ") + TypeName<Ts>::value));
}
template<typename ...Ts, std::enable_if_t<sizeof...(Ts) == 0, bool> = 1> // overload to match empty Ts
constexpr auto typeList() {
    return mkfs("");
}

template<typename R, typename ...Args>
struct TypeName<R(Args...)> {
    static constexpr auto value = mkfs("function returning ") + TypeName<R>::value +
                                    mkfs(" with argument (") + typeList<Args...>() + mkfs(")");
};

// templates
CLST_MAKE_TEMPLATE_NAME_SPECIALIZATION(std::initializer_list)

template<typename ...Ts>
struct TypeName<std::tuple<Ts...>> {
    static constexpr auto value = mkfs("std::tuple of [") + typeList<Ts...>() + mkfs("]");
};

template<typename V, typename ...Rem>
struct TypeName<std::vector<V, Rem...>> { // discard other template parameters
    static constexpr auto value = mkfs("std::vector<") + TypeName<V>::value + mkfs(">");
};

template<template<typename ...> typename T, typename ...Ps>
struct TypeName<T<Ps...>> {
    static constexpr auto value = TemplateName<T>::value +
    mkfs("<") + typeList<Ps...>() + mkfs(">");
};

#undef CLST_MAKE_TYPE_NAME_SPECIALIZATION
#undef CLST_MAKE_TYPE_NAME_SPECIALIZATION_WITH_NAME
#undef CLST_MAKE_TEMPLATE_NAME_SPECIALIZATION
#undef CLST_MAKE_TEMPLATE_NAME_SPECIALIZATION_WITH_NAME

} // namespace typename_details

template<typename T>
constexpr auto type_name_fstr = typename_details::TypeName<T>::value;

template<typename T>
constexpr auto type_name_str = type_name_fstr<T>.str();

} // namespace clst::strings


#endif /* CLST_STRINGS_TYPE_NAME_H */
