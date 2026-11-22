#include "slate_utils.hpp"
#include <slate/log.hpp>

#include <string_view>
#include <print>
#include <ranges>
#include <numeric>
#include <format>
#include <functional>
#include <iterator>
#include <array>
#include <cstdlib>  // getenv

using namespace slate;

namespace {

struct Applet {
    using Fn = int(int, char**);
    std::string_view name;
    Fn& func;
};

constexpr Applet applets[] {
    { "inspect"           , main_slate              },
    { "mp4join"           , main_mp4join            },
    { "insta360_normalize", main_insta360_normalize },
    { "calibrator"        , main_calibrator         }
};

// Build a compile-time string listing all the applet names.
constexpr auto applets_list = [] -> std::string_view {
    // auto len = std::ranges::fold_left(applets, 0, [](int sum, const Applet& a){ return sum + a.name.size(); });
    static constexpr auto bufsize = std::transform_reduce(std::begin(applets), std::end(applets),
                                                          0uz, std::plus<>{},
                                                          [](const Applet& a){ return a.name.size() + 2; }) - 2;
    static constexpr auto pair = []{
        std::array<char, bufsize> arr{};
        const auto end = std::ranges::copy(
            applets
            | std::views::transform([](const auto& fn){return fn.name;})
            | std::views::join_with(std::string_view(", "))
            , arr.begin()
        ).out;
        return std::pair{arr, end - arr.begin()};
    }();
    // static_assert(pair.second == bufsize); // bufsize is exact!
    return {pair.first.data(), pair.second};
}();

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::println(stderr, "Specify an applet to run.");
        std::println(stderr, "Available applets: {}.", applets_list);
        return 2;
    }
    int hit = 0;
    std::string_view arg = argv[1];
    const Applet* target = nullptr;
    for (const auto& applet : applets) {
        if (applet.name.starts_with(arg)) { // Allow unique partial matches.
            target = &applet;
            hit += 1;
        }
    }

    const auto& env_key = "SLATE_LOG_LEVEL";
    log::Level log_level = log::Level::warn;
    if (const auto env_val = std::getenv(env_key)) {
        if (const auto lvl = log::find_level(env_val)) {
            log_level = *lvl;
        }
    }
    log::get_sink().level = log_level;
    SLATE_LOG_DEBUG("Slate log level set to {}", log::label(log_level));

    SLATE_LOG_DEBUG("Prefix \"{}\" match count: {}.", arg, hit);
    if (hit != 1) {
        if (hit > 1) {
            std::println(stderr, "Ambiguous applet name: \"{}\"", arg);
        } else {
            std::println(stderr, "Unknown applet name: \"{}\"", arg);
        }
        std::println(stderr, "Available applets: {}.", applets_list);
        return 2;
    }

    SLATE_LOG_DEBUG("Entering applet \"{}\"...", target->name);
    return target->func(argc - 1, argv + 1);
}
