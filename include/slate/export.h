#ifndef SLATE_EXPORT_H
#define SLATE_EXPORT_H

// API export macros
// define SLATE_SOURCE when building this library
// define SLATE_SHARED_LIB if building as DSO, or when consuming the DSO

#if defined(SLATE_SHARED_LIB)
    #if defined(_WIN32)
        #if defined(SLATE_SOURCE)
            #define SLATE_EXPORT __declspec(dllexport)
        #else
            #define SLATE_EXPORT __declspec(dllimport)
        #endif
    #else
        #define SLATE_EXPORT __attribute__((visibility("default")))
    #endif
#else
    #define SLATE_EXPORT
#endif

#endif /* SLATE_EXPORT_H */
