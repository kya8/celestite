#ifndef CLST_META_TYPE_HOLDER_H
#define CLST_META_TYPE_HOLDER_H

#include <type_traits>

namespace clst::meta {

//TODO: subscript accessor

template <auto Val>
struct val_holder {
    using value_type = decltype(Val);
    static constexpr auto value = Val;

    constexpr operator value_type() const noexcept {
        return Val;
    }
};

template <typename ...Ts>
struct type_holder {
    static constexpr auto size = sizeof...(Ts);

    template<typename F>
    static void apply(F&& f) {
        (void(f.template operator()<Ts>()), ...);
    }
};

template <auto ...Vs>
struct heterogeneous_nontype_holder {
    static constexpr auto size = sizeof...(Vs);

    template<typename F>
    static void apply(F&& f) {
        (void(f.template operator()<Vs>()), ...); // template<auto v> operator();
    }
};

namespace details {

template<typename T, T ...Vs>
struct static_store {
    static constexpr T data_[] {Vs...};
};

} /* details ns */

template <typename T, T ...Vs>
struct homogeneous_nontype_holder {
    using value_type = T;
    static constexpr auto size = sizeof...(Vs);
    // optional storage:
    //static constexpr T data_[] {Vs...};

    template<typename F>
    static void apply(F&& f) {
        (void(f.template operator()<Vs>()), ...);
    }

    template<typename F>
    static void apply2(F&& f) {
        (void(f(val_holder<Vs>{})), ...);  // Vs can be catched by a generic lambda
    }

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

};

// This feels better
template<auto ...Vs>
using to_homogeneous_nontype_holder = homogeneous_nontype_holder<std::common_type_t<decltype(Vs)...>, Vs...>;

} /* namespace meta */

#endif /* CLST_META_TYPE_HOLDER_H */
