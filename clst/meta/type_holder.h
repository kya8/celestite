#ifndef CLST_META_TYPE_HOLDER_H
#define CLST_META_TYPE_HOLDER_H

namespace clst::meta {

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

template <typename T, T ...Vs>
struct homogeneous_nontype_holder {
    using value_type = T;
    static constexpr auto size = sizeof...(Vs);

    template<typename F>
    static void apply(F&& f) {
        (void(f.template operator()<Vs>()), ...);
    }

    template<typename F>
    static void apply2(F&& f) {
        (void(f(val_holder<Vs>{})), ...);  // Vs can be catched by a generic lambda
    }
};

}

#endif /* CLST_META_TYPE_HOLDER_H */
