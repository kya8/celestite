#ifndef LFS_HPP_DE54052E_2D19_4622_990D_9653B2C8C00B
#define LFS_HPP_DE54052E_2D19_4622_990D_9653B2C8C00B

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

namespace slate::detail {

#ifdef _WIN32
inline auto& fseek64 = _fseeki64;
inline auto& ftell64 = _ftelli64;
#else // POSIX
inline auto& fseek64 = fseeko;
inline auto& ftell64 = ftello;
#endif

} // namespace slate::detail

#endif /* LFS_HPP_DE54052E_2D19_4622_990D_9653B2C8C00B */
