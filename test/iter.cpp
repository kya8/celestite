#include <gtest/gtest.h>
#include <clst/iter/nd.h>
#include <vector>
#include <algorithm>

using namespace clst::iter;

TEST(Iter, NdIndex)
{
    constexpr unsigned I = 3,
                       J = 6,
                       K = 10;

    std::vector<std::array<std::size_t, 3>> data_ref, data;

    for (unsigned i = 0; i < I; ++i) {
        for (unsigned j = 0; j < J; ++j) {
            for (unsigned k = 0; k < K; ++k) {
                data_ref.push_back({i, j, k});
            }
        }
    }

    for (auto& [i, j, k] : NdIndex(I, J, K)) {
        ASSERT_TRUE(i<I && j<J && k<K) << "Indexes are out of bound!";
        data.push_back({i, j, k});
    }

    ASSERT_EQ(data_ref.size(), data.size());
    EXPECT_TRUE(
        std::equal(data_ref.cbegin(), data_ref.cend(), data.cbegin())
    );
}
