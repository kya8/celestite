#include <clst/arena.hpp>
#include "test_macros.h"

namespace {

struct alignas(16) TestAlign {};

}

int arena(int, char*[])
{
    static constexpr int len = 32;
    alignas(16) static unsigned char buf[len];
    clst::Arena arena(buf, len);
    CLST_ASSERT(arena.capacity() == len);

    CLST_ASSERT(arena.allocate(1, 2) != nullptr);
    CLST_ASSERT(arena.allocate<int>() != nullptr);
    CLST_ASSERT(arena.allocate<TestAlign>() != nullptr);
    CLST_ASSERT(arena.capacity() == 0);
    CLST_ASSERT(arena.allocate(1, 1) == nullptr);
    
    return 0;
}
