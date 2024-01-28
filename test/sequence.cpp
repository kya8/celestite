#include <clst/meta/sequence.h>
#include <gtest/gtest.h>
#include <sstream>

using namespace clst::meta;

TEST(Sequence, ForSeqBasic)
{
    std::ostringstream s_seq, s_loop;
    static constexpr auto nb_loops = 10;

    for_seq<nb_loops>::apply([&s_seq](auto i) {
        if constexpr (i.value == 999) static_assert(false); // removed at compile time
        if constexpr (i.value == 0) s_seq << "Start!\n";
        s_seq << "loop: " << i.value << ' ';
    });

    for(auto i = 0; i < nb_loops; ++i) {
        if (i == 0) s_loop << "Start!\n";
        s_loop << "loop: " << i << ' ';
    }

    EXPECT_EQ(s_seq.str(), s_loop.str());
}
