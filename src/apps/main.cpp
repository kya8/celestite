#include "applets.hpp"
#include <clst/log.hpp>
#include "string_utils.hpp"
#include "version.hpp"

#include <string_view>
#include <string>
#include <print>
#include <ranges>
#include <algorithm>
#include <format>
#include <iterator>
#include <array>
#include <cstdlib>  // getenv
#include <cassert>

using namespace clst;

namespace {

// Check for applet names.
static_assert(std::ranges::all_of(applets,
    // A condition that must evaluate to true for all applets.
    [](const Applet& a) {
        // Or use ranges::concat from C++26.
        std::string allowed_chars = "_-";
        auto a2z = std::views::iota('a', static_cast<char>('z' + 1));
        allowed_chars.append_range(a2z);
        //allowed_chars.append_range(std::views::iota('A', static_cast<char>('Z' + 1)));
        allowed_chars.append_range(std::views::iota('0', static_cast<char>('9' + 1)));
        return std::ranges::all_of(a.name, [&](char c){ return std::ranges::contains(allowed_chars, c); })
               && std::ranges::contains(a2z, a.name.front());
    }
), "Applet names failed checks.");

// Owning wrapper for a null-terminated string literal, so that it can be used as NTTP
template<std::size_t N>
requires (N > 0)
struct StringLiteral {
    // String size without the terminating '\0'.
    static constexpr auto size = N - 1;
    constexpr StringLiteral(const char(&s)[N]) noexcept {
        std::ranges::copy(s, cstr);
    }
    [[nodiscard]] constexpr std::string_view view() const noexcept {
        return {cstr, size};
    }
    char cstr[N]{}; // null-terminated static string buffer.
};

/**
 * Build a compile-time string listing all the applet names.
 * @tparam Sep The seperator used to join the strings.
 * @tparam Append Whether to append `Sep` after the last char.
 */
template<StringLiteral Sep, bool Append = false>
constexpr std::string_view make_applets_list() {
    // auto len = std::ranges::fold_left(applets, 0, [](int sum, const Applet& a){ return sum + a.name.size(); });
    // static constexpr auto bufsize = std::transform_reduce(std::begin(applets), std::end(applets),
    //                                                       0uz, std::plus<>{},
    //                                                       [](const Applet& a){ return a.name.size() + 2; }) - 2;
    static constexpr auto char_range = applets
                                | std::views::transform(&Applet::name) // The member pointer fn returns the member by reference (const std::string_view&).
                                                                       // If the transform yields by value, `join_with` caches the current string_view
                                                                       // by value, making itself non-const.
                                | std::views::join_with(Sep.view());
    static constexpr auto arr = []{
        std::array<char, std::ranges::distance(char_range) + Append * Sep.size> arr{};
        const auto out = std::ranges::copy(char_range, arr.begin()).out;
        if constexpr (Append) {
            std::ranges::copy(Sep.view(), out);
        }
        return arr;
    }();
    return std::string_view{arr};
};

constexpr auto applets_list = make_applets_list<", ">();
constexpr auto applets_print = make_applets_list<"\n", true>();

using log::Level;
// Default log level integer when verbose is 0.
constexpr auto v0_log_level = CLST_LOG_LEVEL_WARN;
constexpr auto max_verbosity = v0_log_level - CLST_LOG_LEVEL_MIN;
constexpr Level v_to_log_level(int verbosity) noexcept {
    return log::to_level(v0_log_level - verbosity);
}

constexpr std::string_view base_name = "clst";

void print_help() {
    std::print(
R"^^({0}: Unified command-line applets dispatcher.

Usage: {0} [GLOBAL_OPTIONS...] <applet> [APPLET_OPTIONS...]

Available applets: {1}.

Global options:
  -l, --list-applets  Print all available applets, one per line
  -v, --verbose       Increase verbosity level; can be repeated; also applies to applets
  -h, --help          Show help message
  -V, --version       Display version information

Hint: A partial prefix can be used to select the applet, provided that the prefix is unique.
)^^",
    base_name, applets_list);
}

} // namespace

int main(int argc, char** argv)
{
    g_verbosity = 0;
    int ix_global_opt_end = 1;
    for (; ix_global_opt_end < argc; ++ix_global_opt_end) {
        const std::string_view arg = argv[ix_global_opt_end];
        using util::match_any;
        if (arg.front() != '-') {
            break;
        } else if (match_any(arg, "-l", "--list-applets")) {
            std::print(applets_print);
            return 0;
        } else if (match_any(arg, "-h", "--help")) {
            print_help();
            return 0;
        } else if (match_any(arg, "-V", "--version")) {
            version::print_info();
            return 0;
        } else if (match_any(arg, "-v", "--verbose")) {
            g_verbosity += (g_verbosity < max_verbosity);
        } else {
            std::println(stderr, "Invalid global option: {}", arg);
            return 2;
        }
    }
    if (ix_global_opt_end == argc) {
        std::println(stderr, "Specify an applet to run. Or pass '-h' for help.\nAvailable applets: {}.", applets_list);
        return 2;
    }

    const std::string_view arg1 = argv[ix_global_opt_end];
    int hit = 0;
    const Applet* target = nullptr;
    for (const auto& applet : applets) {
        if (applet.name.starts_with(arg1)) { // Allow unique partial matches.
            if (++hit > 1) {
                break;
            }
            target = &applet;
        }
    }
    if (hit != 1) {
        if (hit > 1) {
            std::println(stderr, "Ambiguous applet name prefix: \"{}\"", arg1);
        } else {
            std::println(stderr, "Unknown applet name: \"{}\"", arg1);
        }
        std::println(stderr, "Available applets: {}.", applets_list);
        return 2;
    }
    assert(target != nullptr);

    const Level log_level = []{
        constexpr const auto& env_key = "CLST_LOG_LEVEL";
        // Log level from env var overrides log level mapped from verbosity.
        if (const auto env_val = std::getenv(env_key)) {
            if (const auto lvl = log::find_level(env_val)) {
                return *lvl;
            }
            //CLST_LOG_WARN("env {}={} is not a known log level", env_key, env_val);
        }
        return v_to_log_level(g_verbosity);
    }();
    log::get_sink().level = log_level;
    CLST_LOG_DEBUG("Clst runtime log level set to {}. Compile-time log level is {}", log::label(log_level), log::label(log::to_level(CLST_LOG_LEVEL)));
    CLST_LOG_INFO("Verbosity={}.", g_verbosity);

    CLST_LOG_DEBUG("Entering applet \"{}\"...", target->name);
    return target->func(argc - ix_global_opt_end, argv + ix_global_opt_end);
}
