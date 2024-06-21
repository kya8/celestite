#ifndef CLST_UTIL_ENDIAN_RW_H
#define CLST_UTIL_ENDIAN_RW_H

#include <type_traits>
#include <utility>            // index_sequence
#include <climits>            // CHAR_BIT
#include "clst/util/endian.h" // We don't want to rely on this, however it's still necessary for non-integral types.

namespace clst::util {

namespace details {

// padding bits? Not supported
// FIXME: Bytes < sizeof(T) does not work for signed integers

template<bool BE, typename T, std::size_t ...Is, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
inline T readEndianImpl(const unsigned char* data, std::index_sequence<Is...>) noexcept
{
    if constexpr (std::is_integral_v<T>) {
        using UT = std::make_unsigned_t<T>;
        const UT u = ((static_cast<UT>(data[Is]) << CHAR_BIT * []{if constexpr (BE) return (sizeof...(Is) - 1 - Is); else return Is;}())|...);
        return reinterpret_cast<const T&>(u);
    }
    else {
        static_assert(sizeof...(Is) == sizeof(T));
        T val;
        ((reinterpret_cast<unsigned char*>(&val)[Is] = data[([]{if constexpr ((CLST_ENDIAN == CLST_BIG_ENDIAN) == BE) return Is; else return (sizeof...(Is) - 1 - Is);}())]),...);
        return val;
    }
}

template<bool BE, typename T, std::size_t ...Is, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
inline void writeEndianImpl(const T val, unsigned char* data, std::index_sequence<Is...>) noexcept
{
    if constexpr (std::is_integral_v<T>) {
        using UT = std::make_unsigned_t<T>;
        const UT u = reinterpret_cast<const UT&>(val);
        ((data[Is] = u >> CHAR_BIT * []{if constexpr (BE) return (sizeof...(Is) - 1 - Is); else return Is;}()),...);
    }
    else {
        static_assert(sizeof...(Is) == sizeof(T));
        ((data[Is] = reinterpret_cast<const unsigned char*>(&val)[([]{if constexpr ((CLST_ENDIAN == CLST_BIG_ENDIAN) == BE) return Is; else return (sizeof...(Is) - 1 - Is);}())]),...);
    }
}

} // namespace details

template<typename T, unsigned Bytes = sizeof(T)>
inline T readBE(const void* data) noexcept
{
    return details::readEndianImpl<1, T>(static_cast<const unsigned char*>(data), std::make_index_sequence<Bytes>{});
}

template<typename T, unsigned Bytes = sizeof(T)>
inline T readLE(const void* data) noexcept
{
    return details::readEndianImpl<0, T>(static_cast<const unsigned char*>(data), std::make_index_sequence<Bytes>{});
}

template<typename T, unsigned Bytes = sizeof(T)>
inline void writeBE(const T val, void* data) noexcept
{
    return details::writeEndianImpl<1>(val, static_cast<unsigned char*>(data), std::make_index_sequence<Bytes>{});
}

template<typename T, unsigned Bytes = sizeof(T)>
inline void writeLE(const T val, void* data) noexcept
{
    return details::writeEndianImpl<0>(val, static_cast<unsigned char*>(data), std::make_index_sequence<Bytes>{});
}

} // namespace clst::util

#endif /* CLST_UTIL_ENDIAN_RW_H */
