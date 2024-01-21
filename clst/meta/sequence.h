#ifndef CLST_META_SEQUENCE_H
#define CLST_META_SEQUENCE_H

#include <utility>
#include <type_traits>

namespace clst::meta {

template<typename>
struct for_ints;

template<typename IT, IT ...Is>
struct for_ints<std::integer_sequence<IT, Is...>> {  // maybe inherit from homogeneous_nontype_holder?

    static void apply(F&& f) {
        (void(f(std::integral_constant<IT, Is>{})), ...);
    }

};

template<auto N>
using for_seq = for_ints<std::make_integer_sequence<decltype(N), N>>;


// Another integer_sequence implementation
template<typename IT, IT ...>
struct int_seq {};

namespace details {

template<typename, typename>
struct concat_ints;

template<typename IT, IT ...Is1, IT ...Is2>
struct concat_ints<int_seq<IT, Is1...>, int_seq<IT, Is2...>> {
    using type = int_seq<IT, Is1..., (sizeof...(Is1) + Is2)...>;
};

template<typename IT, IT N, typename = void>
struct make_ints : concat_ints<typename make_ints<IT, N/2>::type, typename make_ints<IT, N - N/2>::type> {};

template<typename IT, IT N>
struct make_ints<IT, N, std::enable_if_t<N==0>> {
    using type = int_seq<IT>;
};

template<typename IT, IT N>
struct make_ints<IT, N, std::enable_if_t<N==1>> {
    using type = int_seq<IT, 0>;
};

} /* details ns */

template<typename IT, IT N>
using make_int_seq = typename make_ints<IT, N>::type;

}

#endif /* CLST_META_SEQUENCE_H */
