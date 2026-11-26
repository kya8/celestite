#ifndef CLST_TAGS_HPP
#define CLST_TAGS_HPP

// Tags types

namespace clst {

// Tag type to select a non-throwing overload
struct NoEx {};
using NoExcept = NoEx;
inline constexpr NoEx noex;

// Tag type to select a throwing overload
struct Ex {};
inline constexpr Ex ex;

constexpr Ex operator!(NoEx) noexcept { return {}; }

constexpr NoEx operator!(Ex) noexcept { return {}; }

} // namespace clst

#endif /* CLST_TAGS_HPP */
