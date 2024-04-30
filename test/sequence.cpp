#include <clst/meta/sequence.h>
#include <gtest/gtest.h>
#include <sstream>
#include <clst/meta/traits.h> // false_v

using namespace clst::meta;

TEST(Sequence, ForSeqBasic)
{
    std::ostringstream s_seq, s_loop;
    static constexpr auto nb_loops = 10;

    ForSeq<nb_loops>::apply([&s_seq](auto i) {
        if constexpr (i.value == 999) static_assert(traits::false_v<decltype(i)>); // removed at compile time
        if constexpr (i.value == 0) s_seq << "Start!\n";
        s_seq << "loop: " << i.value << ' ';
    });

    for(auto i = 0; i < nb_loops; ++i) {
        if (i == 0) s_loop << "Start!\n";
        s_loop << "loop: " << i << ' ';
    }

    EXPECT_EQ(s_seq.str(), s_loop.str());
}
