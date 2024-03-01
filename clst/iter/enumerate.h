#ifndef CLST_ITER_ENUMERATE_H
#define CLST_ITER_ENUMERATE_H

#include <utility>     // pair, declval
#include <type_traits>
#include <iterator>    // iterator_traits, std::begin/end

namespace clst::iter
{

namespace details
{

template <typename T, typename = std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>>
constexpr auto enumerate(T&& t)
{
    using IterT = decltype(std::begin(std::declval<T>()));
    using IterTraits = std::iterator_traits<IterT>;
    struct EnumerateIterator {
        typename IterTraits::difference_type index_;
        IterT it_;

        auto operator*() const { // This returns a temporary value, not by reference.
            return std::pair<decltype(index_), typename IterTraits::reference>(index_, *it_);
        }
        bool operator==(const EnumerateIterator& rhs) const {
            return it_ == rhs.it_;
        }
        bool operator!=(const EnumerateIterator& rhs) const {
            return it_ != rhs.it_;
        }
        auto& operator++() {
            ++index_;
            ++it_;
            return *this;
        }
    };

    struct EnumerateIterableProxy {
        T t_; // rvalue t will be moved in.
              // Cannot use T&& here, since "a temporary bound to a reference parameter in a function call exists until the end of the full expression containing that function call",
              // so enumerate(rvalue_here()) won't work as expected.

        EnumerateIterator begin() {
            return {0, std::begin(t_)};
        }
        EnumerateIterator end() {
            return {-1, std::end(t_)};
        }
    };
    return EnumerateIterableProxy{std::forward<T>(t)};
}

} // namespace details

template <typename T>
constexpr auto enumerate(T&& t)
{
    return details::enumerate(std::forward<T>(t));
}


} // namespace clst::iter


#endif /* CLST_ITER_ENUMERATE_H */


// Some tests
#if 0

int main()
{
    using clst::iter::enumerate;
    std::vector v{11,22,33};
    for (auto&&[i, val] : enumerate(v)) {
        val = i;
    }
    fmt::println("{}", v);

    for (auto&&[i, val] : enumerate(std::vector{11,22,33})) {
        fmt::println("{}: {}", i ,val);
    }

    for (auto [i,c] : enumerate("hello")) {
        fmt::println("{}: {}", i, c);
    }
}

#endif
