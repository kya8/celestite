#ifndef CLST_TYPES_INTS_H
#define CLST_TYPES_INTS_H


#include <cstdint>
#include <cstddef>

namespace clst::types::ints {

using i8  = std::int_least8_t;
using i16 = std::int_least16_t;
using i32 = std::int_least32_t;
using i64 = std::int_least64_t;
using u8  = std::uint_least8_t;
using u16 = std::uint_least16_t;
using u32 = std::uint_least32_t;
using u64 = std::uint_least64_t;

using usize = std::size_t;

using uchar = unsigned char;
using schar = signed char;

} /* namespace ints */


#endif /* CLST_TYPES_INTS_H */
