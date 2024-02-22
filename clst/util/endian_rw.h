#ifndef CLST_UTIL_ENDIAN_RW_H
#define CLST_UTIL_ENDIAN_RW_H

#include <type_traits>
#include <utility>
#include <climits>

namespace clst::util {

namespace details {

// padding bits?
// TODO: floating point types.

template<bool BE, typename T, std::size_t ...Is, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
inline T read_endian_impl(const unsigned char* data, std::index_sequence<Is...>) noexcept
{
    if constexpr (std::is_integral_v<T>) {
        using UT = std::make_unsigned_t<T>;
        const UT u = ((static_cast<UT>(data[Is]) << CHAR_BIT * []{if constexpr (BE) return (sizeof...(Is) - 1 - Is); else return Is;}())|...);
        return reinterpret_cast<const T&>(u);
    }
    else {
        // floats
    }
}

template<bool BE, typename T, std::size_t ...Is, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
inline void write_endian_impl(const T val, unsigned char* data, std::index_sequence<Is...>) noexcept
{
    if constexpr (std::is_integral_v<T>) {
        using UT = std::make_unsigned_t<T>;
        const UT u = reinterpret_cast<const UT&>(val);
        ((data[Is] = u >> CHAR_BIT * []{if constexpr (BE) return (sizeof...(Is) - 1 - Is); else return Is;}()),...);
    }
    else {
        // floats
    }
}

} // namespace details

template<typename T, unsigned Bytes = sizeof(T)>
inline T read_BE(const void* data) noexcept
{
    return details::read_endian_impl<1, T>(static_cast<const unsigned char*>(data), std::make_index_sequence<Bytes>{});
}

template<typename T, unsigned Bytes = sizeof(T)>
inline T read_LE(const void* data) noexcept
{
    return details::read_endian_impl<0, T>(static_cast<const unsigned char*>(data), std::make_index_sequence<Bytes>{});
}

template<typename T, unsigned Bytes = sizeof(T)>
inline void write_BE(const T val, void* data) noexcept
{
    return details::write_endian_impl<1>(val, static_cast<unsigned char*>(data), std::make_index_sequence<Bytes>{});
}

template<typename T, unsigned Bytes = sizeof(T)>
inline void write_LE(const T val, void* data) noexcept
{
    return details::write_endian_impl<0>(val, static_cast<unsigned char*>(data), std::make_index_sequence<Bytes>{});
}

} // namespace clst::util

#endif /* CLST_UTIL_ENDIAN_RW_H */
