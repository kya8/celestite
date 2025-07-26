#ifndef CLST_ENDIAN_H
#define CLST_ENDIAN_H

#define CLST_LITTLE_ENDIAN  0
#define CLST_BIG_ENDIAN     1

// Credit: adapted from rapidjson.h

#ifndef CLST_ENDIAN
#  ifdef __BYTE_ORDER__
#    if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#      define CLST_ENDIAN CLST_LITTLE_ENDIAN
#    elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#      define CLST_ENDIAN CLST_BIG_ENDIAN
#    else
#      error Could not detect endianness. You should specify CLST_ENDIAN.
#    endif // __BYTE_ORDER__
// Detect with GLIBC's endian.h
#  elif defined(__GLIBC__)
#    include <endian.h>
#    if (__BYTE_ORDER == __LITTLE_ENDIAN)
#      define CLST_ENDIAN CLST_LITTLE_ENDIAN
#    elif (__BYTE_ORDER == __BIG_ENDIAN)
#      define CLST_ENDIAN CLST_BIG_ENDIAN
#    else
#      error Could not detect endianness. You should specify CLST_ENDIAN.
#   endif // __GLIBC__
// Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
#  elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#    define CLST_ENDIAN CLST_LITTLE_ENDIAN
#  elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#    define CLST_ENDIAN CLST_BIG_ENDIAN
// Detect with architecture macros
#  elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) || defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(_POWER) || defined(__s390__)
#    define CLST_ENDIAN CLST_BIG_ENDIAN
#  elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#    define CLST_ENDIAN CLST_LITTLE_ENDIAN
#  elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#    define CLST_ENDIAN CLST_LITTLE_ENDIAN
#  else
#    error Could not detect endianness. You should specify CLST_ENDIAN.
#  endif
#endif // CLST_ENDIAN

#endif // CLST_ENDIAN_H
