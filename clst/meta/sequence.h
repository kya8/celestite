#ifndef CLST_META_SEQUENCE_H
#define CLST_META_SEQUENCE_H

#include <utility>
#include <type_traits>

namespace clst::meta {

template<typename>
struct for_ints;

template<typename IntT, IntT ...Is>
struct for_ints<std::integer_sequence<IntT, Is...>> {  // maybe inherit from uni_list?

    template<typename F>
    static constexpr void apply(F&& f) {
        (void(f(std::integral_constant<IntT, Is>{})), ...);
    }

};

template<auto N>
using for_seq = for_ints<std::make_integer_sequence<decltype(N), N>>;


// Another integer_sequence implementation
template<typename IntT, IntT ...>
struct int_seq {};

namespace details {

template<typename, typename>
struct concat_ints;

template<typename IntT, IntT ...Is1, IntT ...Is2>
struct concat_ints<int_seq<IntT, Is1...>, int_seq<IntT, Is2...>> {
    using type = int_seq<IntT, Is1..., (sizeof...(Is1) + Is2)...>;
};

template<typename IntT, IntT N, typename = void>
struct make_ints : concat_ints<typename make_ints<IntT, N/2>::type, typename make_ints<IntT, N - N/2>::type> {};

template<typename IntT, IntT N>
struct make_ints<IntT, N, std::enable_if_t<N==0>> {
    using type = int_seq<IntT>;
};

template<typename IntT, IntT N>
struct make_ints<IntT, N, std::enable_if_t<N==1>> {
    using type = int_seq<IntT, 0>;
};

} /* details ns */

template<typename IntT, IntT N>
using make_int_seq = typename details::make_ints<IntT, N>::type;

}

#endif /* CLST_META_SEQUENCE_H */

#if 0
// A test case
int main() {
    using clst::meta::for_seq;
    for_seq<10>::apply([](auto i) {
        if constexpr (i.value == 999) static_assert(false); // removed at compile time
        if constexpr (i.value == 0)   std::cout << "Start!\n";
        std::cout << "loop: " << i.value << ' ';
    });
}

#endif
