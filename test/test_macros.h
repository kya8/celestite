#ifndef CLST_TEST_MACROS_H
#define CLST_TEST_MACROS_H

#include <stdlib.h> // exit
#include <stdio.h>  // fprintf

#define CLST_QUOTE(str) #str
#define CLST_STR(str) CLST_QUOTE(str)

#define CLST_ASSERT(COND)                                                                                                   \
    do {                                                                                                                    \
        if (!(COND)) {                                                                                                      \
            fprintf(stderr, "%s:%s: %s: Assertion `%s' failed!\n", __FILE__, CLST_STR(__LINE__), __func__, CLST_STR(COND)); \
            exit(EXIT_FAILURE);                                                                                             \
        }                                                                                                                   \
    } while (0)

#define CLST_ASSERT_EQ(L, R) CLST_ASSERT((L) == (R))

#ifdef __cplusplus

#define CLST_EXPECT_THROW(EXPR, EXCEPTION)                                                                                           \
    do {                                                                                                                             \
        bool ok = false;                                                                                                             \
        try {                                                                                                                        \
            EXPR;                                                                                                                    \
        } catch (const EXCEPTION&) {                                                                                                 \
            ok = true;                                                                                                               \
        } catch (...) {}                                                                                                             \
        if (!ok) {                                                                                                                   \
            fprintf(stderr, "%s:%s: Expected exception `%s' was not thrown!\n", __FILE__, CLST_STR(__LINE__), CLST_STR(EXCEPTION));  \
            exit(EXIT_FAILURE);                                                                                                      \
        }                                                                                                                            \
    } while (0)

#define CLST_EXPECT_NOTHROW(EXPR)                                                                   \
    do {                                                                                            \
        try {                                                                                       \
            EXPR;                                                                                   \
        } catch (...) {                                                                             \
            fprintf(stderr, "%s:%s: Unexpected exception caught!\n", __FILE__, CLST_STR(__LINE__)); \
            exit(EXIT_FAILURE);                                                                     \
        }                                                                                           \
    } while (0)

#endif

#endif // CLST_TEST_MACROS_H
