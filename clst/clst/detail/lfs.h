#ifndef CLST_LFS_H
#define CLST_LFS_H

// Wrappers for 64-bit fseek/ftell

#ifdef _WIN32
#include <stdint.h>
#else // Assume POSIX
#define _FILE_OFFSET_BITS 64
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include <stdio.h>

#ifdef __cplusplus

namespace clst::detail {

#ifdef _WIN32
inline auto fseek64(FILE* stream, int64_t offset, int whence) {
    return _fseeki64(stream, offset, whence);
}
inline auto ftell64(FILE* stream) {
    return _ftelli64(stream);
}
#else
inline auto fseek64(FILE* stream, off_t offset, int whence) {
    return fseeko(stream, offset, whence);
}
inline auto ftell64(FILE* stream) {
    return ftello(stream);
}
#endif

}

#else // C code

#ifdef _WIN32
inline int clst_fseek64(FILE* stream, int64_t offset, int whence) {
    return _fseeki64(stream, offset, whence);
}
inline int64_t clst_ftell64(FILE* stream) {
    return _ftelli64(stream);
}
#else
inline int clst_fseek64(FILE* stream, off_t offset, int whence) {
    return fseeko(stream, offset, whence);
}
inline off_t clst_ftell64(FILE* stream) {
    return ftello(stream);
}
#endif

#endif // __cplusplus

#endif // CLST_LFS_H
