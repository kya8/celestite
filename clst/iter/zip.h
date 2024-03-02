#ifndef CLST_ITER_ZIP_H
#define CLST_ITER_ZIP_H

#include <tuple>

namespace clst::iter
{

template <typename Iter0, typename ...Iters>
constexpr auto zip(Iter0 begin0, Iter0 end0, Iters ...Begins)
{
    struct ZipIterator {
        Iter0 iter0_; // the pivot
        std::tuple<Iters...> iters_;
        auto operator*() const {
            return std::apply([this](auto&& ...iters) {
                return std::tuple<decltype(*begin0), decltype(*Begins)...>{*iter0_, (*iters)...};
            },
            iters_);
        }
        bool operator==(const ZipIterator& rhs) const {
            return iter0_ == rhs.iter0_; // only the pivot is compared!
                                         // Ideally, all iters should be compared, except when iter0 has reached end.
        }
        bool operator!=(const ZipIterator& rhs) const {
            return iter0_ != rhs.iter0_;
        }
        auto& operator++() {
            ++iter0_;
            std::apply([](auto&& ...iters) { ((++iters),...); }, iters_);
            return *this;
        }
    };
    struct ZipProxy {
        Iter0 begin0_, end0_;
        std::tuple<Iters...> iters_;
    
        ZipIterator begin() {
            return {begin0_, iters_};
        }
        ZipIterator end() {
            return {end0_, iters_};
        }
    };
    return ZipProxy{begin0, end0, {Begins...}};
}

} // namespace clst::iter


#endif /* CLST_ITER_ZIP_H */
