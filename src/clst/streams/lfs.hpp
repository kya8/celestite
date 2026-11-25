#ifndef CLST_STREAMS_LFS_HPP
#define CLST_STREAMS_LFS_HPP

#ifndef _WIN32 // Assume POSIX
#define _FILE_OFFSET_BITS 64
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

extern "C" {
#include <stdio.h>
}

// Alias for 64-bit fseek/ftell

namespace clst::detail {

#ifdef _WIN32
inline auto& fseek64 = _fseeki64;
inline auto& ftell64 = _ftelli64;
#else // POSIX
inline auto& fseek64 = fseeko;
inline auto& ftell64 = ftello;
#endif

} // namespace clst::detail

#endif /* CLST_STREAMS_LFS_HPP */
