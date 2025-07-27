#include <clst/scope_guard.hpp>
#include <vector>
#include "test_macros.h"
#include <utility> // std::move

int scope_guard(int, char*[])
{
    std::vector<int> vec;
    {
        auto cb0 = [&] { vec.push_back(0); };
        auto cb1 = [&] { vec.push_back(1); };
        clst::ScopeGuard sg0(cb0);
        clst::ScopeGuard sg1(std::move(cb1));
    }
    CLST_ASSERT((vec == std::vector{1, 0}));

    return 0;
}
