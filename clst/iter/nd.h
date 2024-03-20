#ifndef CLST_ITER_ND_H
#define CLST_ITER_ND_H

#include <array>
#include <utility> // index_sequence

namespace clst::iter {

/**
 * Iterate over N-dimensional index tuple
 */
template <std::size_t Dim>
class NdIndex {
private:
    std::array<std::size_t, Dim> sizes_;
public:
    static constexpr auto ndim = Dim;

    template <typename ...Ts>
    constexpr NdIndex(Ts ...Args) noexcept : sizes_{std::size_t(Args)...} {}

    class Iter {
        friend class NdIndex;
    private:
        std::array<std::size_t, Dim> idx_{};
        std::array<std::size_t, Dim> sizes_;

        // helper for loop unrolling
        template <std::size_t ...Is>
        constexpr void increment(std::index_sequence<Is...>) noexcept {
            ([this] {
                constexpr auto i = Dim - 1 - Is;
                if constexpr (i == 0) {
                    ++idx_[0];
                    return true;
                }
                else {
                    if (++idx_[i] < sizes_[i]) return false;
                    idx_[i] = 0;
                    return true;
                }
            }() && ...); // short-circuit fold
        }
    public:
        constexpr auto& operator*() const noexcept { return idx_; }
        constexpr auto& operator++() noexcept {
            increment(std::make_index_sequence<Dim>{});
            return *this;
        }
        constexpr bool operator==(const Iter& rhs) const noexcept {
            return idx_ == rhs.idx_; // sizes?
        }
        constexpr bool operator!=(const Iter& rhs) const noexcept {
            return !(*this == rhs);
        }
    };

    constexpr auto begin() const noexcept {
        Iter it;
        it.sizes_ = sizes_;
        return it;
    }
    constexpr auto end() const noexcept {
        Iter it;
        it.idx_[0] = sizes_[0];
        it.sizes_ = sizes_;
        return it;
    }
};
// CTAD guide
template<typename ...Ts>
NdIndex(Ts ...Args) -> NdIndex<sizeof...(Ts)>;

} // namespace clst::iter

#endif /* CLST_ITER_ND_H */
