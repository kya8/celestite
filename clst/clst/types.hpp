#ifndef CLST_TYPES_HPP
#define CLST_TYPES_HPP

#include <cstddef>

namespace clst {

using Index = std::ptrdiff_t;
using Size = std::ptrdiff_t;
using Offset = std::ptrdiff_t;

template<typename T>
using OwningPointer = T*;

template<typename T>
using NonOwningPointer = T*;

} // namespace clst

#endif /* CLST_TYPES_HPP */
