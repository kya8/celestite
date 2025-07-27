#include <clst/num_cast.hpp>
#include "test_macros.h"
#include <climits>

// Exposition only

int num_cast(int, char*[])
{
    using clst::num_cast_ex;
    CLST_EXPECT_THROW(num_cast_ex<char>(CHAR_MAX + 1), std::range_error);
    CLST_EXPECT_THROW(num_cast_ex<char>(CHAR_MIN - 1), std::range_error);
    CLST_EXPECT_NOTHROW(CLST_ASSERT_EQ(num_cast_ex<char>(CHAR_MAX), CHAR_MAX));
    CLST_EXPECT_NOTHROW(CLST_ASSERT_EQ(num_cast_ex<char>(CHAR_MIN), CHAR_MIN));

    CLST_EXPECT_THROW(num_cast_ex<int>((unsigned int)(INT_MAX) + 1), std::range_error);
    CLST_EXPECT_THROW(num_cast_ex<unsigned int>(-1), std::range_error);
    CLST_EXPECT_NOTHROW(CLST_ASSERT_EQ(num_cast_ex<int>((unsigned int)INT_MAX), INT_MAX));
    CLST_EXPECT_NOTHROW(CLST_ASSERT_EQ(num_cast_ex<int>((unsigned int)0), 0));
    CLST_EXPECT_NOTHROW(CLST_ASSERT_EQ(num_cast_ex<unsigned int>(INT_MAX), INT_MAX));

    return 0;
}
