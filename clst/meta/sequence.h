#ifndef CLST_META_SEQUENCE_H
#define CLST_META_SEQUENCE_H

#include <utility>
#include <type_traits>

namespace clst::meta {

template<typename>
struct ForInts;

template<typename IntT, IntT ...Is>
struct ForInts<std::integer_sequence<IntT, Is...>> {  // maybe inherit from uni_list?

    template<typename F>
    static constexpr void apply(F&& f) {
        (void(f(std::integral_constant<IntT, Is>{})), ...);
    }

};

template<auto N>
using ForSeq = ForInts<std::make_integer_sequence<decltype(N), N>>;


// Another integer_sequence implementation
template<typename IntT, IntT ...>
struct IntSeq {};

namespace details {

template<typename, typename>
struct ConcatInts;

template<typename IntT, IntT ...Is1, IntT ...Is2>
struct ConcatInts<IntSeq<IntT, Is1...>, IntSeq<IntT, Is2...>> {
    using type = IntSeq<IntT, Is1..., (sizeof...(Is1) + Is2)...>;
};

template<typename IntT, IntT N, typename = void>
struct MakeInts : ConcatInts<typename MakeInts<IntT, N/2>::type, typename MakeInts<IntT, N - N/2>::type> {};

template<typename IntT, IntT N>
struct MakeInts<IntT, N, std::enable_if_t<N==0>> {
    using type = IntSeq<IntT>;
};

template<typename IntT, IntT N>
struct MakeInts<IntT, N, std::enable_if_t<N==1>> {
    using type = IntSeq<IntT, 0>;
};

} /* details ns */

template<typename IntT, IntT N>
using MakeIntSeq = typename details::MakeInts<IntT, N>::type;

}

#endif /* CLST_META_SEQUENCE_H */
