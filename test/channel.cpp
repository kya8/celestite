#include <clst/channel.hpp>
#include "test_macros.h"
#include <string>
#include <vector>
#include <thread>

int channel(int, char*[])
{
    clst::Channel<std::string, true, true, true> ch1(16);
    auto ch2 = clst::make_channel<std::string, true, true>(16);
    std::vector<std::string> result;
    static constexpr auto nb_items = 1000;

    std::thread t0 {
        [&] {
            for (int i = 0; i < nb_items; ++i) {
                ch1.emplace(std::string("data ") += std::to_string(i));
            }
            ch1.close();
        }
    };

    std::thread t1 {
        [&] {
            std::string s;
            while (ch1.pop(s)) {
                ch2.emplace(std::move(s += " processed"));
            }
            ch2.close();
        }
    };

    std::thread t2 {
        [&] {
            std::string s;
            while (ch2.pop(s)) {
                result.push_back(std::move(s));
            }
        }
    };

    t0.join();
    t1.join();
    t2.join();

    CLST_ASSERT(result.size() == nb_items);
    for (int i = 0; i < nb_items; ++i) {
        CLST_ASSERT(result[i] == (std::string("data ") + std::to_string(i) + " processed"));
    }

    return 0;
}
