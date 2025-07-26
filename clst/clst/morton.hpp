#ifndef CLST_MORTON_HPP
#define CLST_MORTON_HPP

#include <cstdint>
#include <array>

namespace clst {

namespace detail {

/**
 * Pad 21 lo bits of x to 63 bits, i.e. 0bxyz -> 0b00x00y00z
 * @pre Input is less than 2^21
 */
inline constexpr std::uint64_t
pad_2bit(std::uint64_t x) noexcept
{
    x = (x | x << 32) & 0x001f00000000ffff;
    x = (x | x << 16) & 0x001f0000ff0000ff;
    x = (x | x << 8 ) & 0x100f00f00f00f00f;
    x = (x | x << 4 ) & 0x10c30c30c30c30c3;
    x = (x | x << 2 ) & 0x1249249249249249;
    return x;
}

inline constexpr std::uint64_t
unpad_2bit(std::uint64_t x) noexcept
{
    x = (x | x >> 2 ) & 0x10c30c30c30c30c3;
    x = (x | x >> 4 ) & 0x100f00f00f00f00f;
    x = (x | x >> 8 ) & 0x001f0000ff0000ff;
    x = (x | x >> 16) & 0x001f00000000ffff;
    x = (x | x >> 32) & 0x00000000001fffff;
    return x;
}

} // namespace detail

/**
 * Make 21x3 interleaved morton code
 * @pre x, y, z are less than 2^21
 */
inline constexpr std::uint64_t
morton_3d(std::uint64_t x, std::uint64_t y, std::uint64_t z) noexcept
{
    using namespace detail;
    return (pad_2bit(x) << 2) | (pad_2bit(y) << 1) | pad_2bit(z);
}

inline constexpr std::array<std::uint64_t, 3>
demorton_3d(std::uint64_t code) noexcept
{
    using namespace detail;
    return
    {
        unpad_2bit(code >> 2 & 0x1249249249249249),
        unpad_2bit(code >> 1 & 0x1249249249249249),
        unpad_2bit(code & 0x1249249249249249)
    };
}

} // namespace clst

#endif // CLST_MORTON_HPP
