#ifndef SLATE_COMMON_PTAG_HPP
#define SLATE_COMMON_PTAG_HPP

namespace util {

/**
 * A tag class for defining priority levels in overload resolution.
 */
template<int P>
requires (P <= 128 && P >= 0)
struct PTag : PTag<P - 1> {
    static constexpr auto level = P;
};

template<>
struct PTag<0> {
    static constexpr auto level = 0;
};

template<int P>
inline constexpr auto ptag = PTag<P>{};
inline constexpr auto ptag_top = ptag<128>;

} // namespace util


#endif /* SLATE_COMMON_PTAG_HPP */
