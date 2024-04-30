#include <gtest/gtest.h>
#include <clst/numerical/num_cast.h>
#include <climits>

using clst::num_cast;
using clst::error::BadNumCast;

TEST(NumCast, IntegralCasts)
{
    EXPECT_THROW(num_cast<char>(CHAR_MAX + 1), BadNumCast);
    EXPECT_THROW(num_cast<char>(CHAR_MIN - 1), BadNumCast);
    EXPECT_NO_THROW(EXPECT_EQ(num_cast<char>(CHAR_MAX), CHAR_MAX));
    EXPECT_NO_THROW(EXPECT_EQ(num_cast<char>(CHAR_MIN), CHAR_MIN));

    EXPECT_THROW(num_cast<int>((unsigned int)(INT_MAX) + 1), BadNumCast);
    EXPECT_THROW(num_cast<unsigned int>(-1), BadNumCast);
    EXPECT_NO_THROW(EXPECT_EQ(num_cast<int>((unsigned int)INT_MAX), INT_MAX));
    EXPECT_NO_THROW(EXPECT_EQ(num_cast<int>((unsigned int)0), 0));
    EXPECT_NO_THROW(EXPECT_EQ(num_cast<unsigned int>(INT_MAX), INT_MAX));
}
