#ifndef CLST_EXPORT_H
#define CLST_EXPORT_H

// API export macros
// define CLST_SOURCE when building this library
// define CLST_SHARED_LIB if building as DSO, or when consuming the DSO

#if defined(CLST_SHARED_LIB)
    #if defined(_WIN32)
        #if defined(CLST_SOURCE)
            #define CLST_EXPORT __declspec(dllexport)
        #else
            #define CLST_EXPORT __declspec(dllimport)
        #endif
    #else
        #define CLST_EXPORT __attribute__((visibility("default")))
    #endif
#else
    #define CLST_EXPORT
#endif

#endif // CLST_EXPORT_H
