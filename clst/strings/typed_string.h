#ifndef CLST_STRINGS_TYPED_STRING_H
#define CLST_STRINGS_TYPED_STRING_H

//#include "clst/meta/traits.h"

namespace clst::strings {

template<char... C>
struct TypeString {
    static constexpr char data[] {C..., 0};
    static constexpr auto size = sizeof...(C);
    static constexpr auto data = &data[0];
    
    constexpr operator const char*() const noexcept { return data; }
};

template<typename, typename>
struct ConcatTypeString;

template<char... C1, char... C2>
struct ConcatTypeString<TypeString<C1...>, TypeString<C2...>> {
    using type = TypeString<C1..., C2...>;
};

namespace details
{

template<unsigned long long N, char...Cs>
constexpr auto
digits_typed_string_impl()
{
  if constexpr (N < 10)
    return TypeString<N+'0', Cs...>{};
  else
    return digits_typed_string_impl<N/10, (N%10)+'0', Cs...>();
}

} // namespace details

template<unsigned long long N>
using digitsTypeString = decltype(details::digits_typed_string_impl<N>());


} // namespace clst::strings


#endif /* CLST_STRINGS_TYPED_STRING_H */
