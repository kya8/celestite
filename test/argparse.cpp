#include "common/argparse.hpp"
#include "test.h"

#include <array>
#include <iterator>
#include <limits>
#include <utility>

namespace {

using namespace clst::cli;

struct Config {
    bool force = false;
    bool legacy_alias = false;
    int verbosity = 0;
    int jobs = 2;
    std::string_view text = "default";
    const char* path = nullptr;
};

using Args = Parser<Config,
    Flag<&Config::force, "-f", "--force">,
    Flag<&Config::legacy_alias, "-d0">,
    CountFlag<&Config::verbosity, 3, "-v", "--verbose">,
    Option<&Config::jobs, "-j", "-jobs", "--jobs">,
    Option<&Config::text, "-d", "--text">,
    Option<&Config::path, "-o", "--output">>;

struct Strings {
    std::string owned;
    std::string_view borrowed;
    const char* pointer = nullptr;
};

using StringArgs = Parser<Strings,
    Option<&Strings::owned, "--owned">,
    Option<&Strings::borrowed, "--borrowed">,
    Option<&Strings::pointer, "-p">>;

struct ByteCounter {
    unsigned char count = std::numeric_limits<unsigned char>::max() - 1;
    const int read_only = 0;
    double floating = 0;
    void function() {}
};

template<class Cfg, class... Bindings>
concept ValidSchema = requires { typename Parser<Cfg, Bindings...>; };

static_assert(!ValidSchema<Config, Flag<&Config::force>>);
static_assert(!ValidSchema<Config, Flag<&Config::force, "force">>);
static_assert(!ValidSchema<Config, Flag<&Config::force, "-">>);
static_assert(!ValidSchema<Config, Flag<&Config::force, "--">>);
static_assert(!ValidSchema<Config, Flag<&Config::force, "--force=on">>);
static_assert(!ValidSchema<Config, Flag<&Config::force, "--two words">>);
static_assert(!ValidSchema<Config, Flag<&Config::force, "--embedded\0nul">>);
static_assert(!ValidSchema<Config, Flag<&Config::force, "-f", "-f">>);
static_assert(!ValidSchema<Config, Flag<&Config::force, "-f">, Option<&Config::jobs, "-f">>);
static_assert(!ValidSchema<Config, Flag<&Config::jobs, "-j">>);
static_assert(!ValidSchema<Config, Option<&Config::force, "-f">>);
static_assert(!ValidSchema<Config, Option<&ByteCounter::count, "-c">>);
static_assert(!ValidSchema<Config, Flag<static_cast<bool Config::*>(nullptr), "-f">>);
static_assert(!ValidSchema<ByteCounter, CountFlag<&ByteCounter::count, -1, "-v">>);
static_assert(!ValidSchema<ByteCounter, Option<&ByteCounter::read_only, "-r">>);
static_assert(!ValidSchema<ByteCounter, Option<&ByteCounter::floating, "-f">>);
static_assert(!ValidSchema<ByteCounter, Option<&ByteCounter::function, "-f">>);

static_assert(noexcept(Args::parse(0, nullptr, std::declval<Config&>())));
static_assert(noexcept(Args::parse(0, nullptr, std::declval<Config&>(), [](const char*) noexcept { return true; })));
static_assert(!noexcept(Args::parse(0, nullptr, std::declval<Config&>(), [](const char*) { return true; })));
static_assert(!noexcept(StringArgs::parse(0, nullptr, std::declval<Strings&>())));

constexpr bool parse_at_compile_time()
{
    Config config;
    const char* const argv[] = {"tool", "-d0", "-vf", "-v", "--verbose", "-vvv", "-j-4", "--text=0,2,3"};
    constexpr auto argc = static_cast<int>(std::size(argv));
    const auto result = Args::parse(argc, argv, config);
    return result && result.index == argc && config.legacy_alias && config.force && config.verbosity == 3 &&
           config.jobs == -4 && config.text == "0,2,3" && config.text.data() == argv[7] + 7;
}

static_assert(parse_at_compile_time());

void test_option_values()
{
    Config config;
    const auto empty = Args::parse(0, nullptr, config);
    CLST_ASSERT(empty);
    CLST_ASSERT_EQ(empty.index, 0);
    CLST_ASSERT_EQ(config.jobs, 2);
    CLST_ASSERT_EQ(config.text, "default");

    using Numeric = Parser<Config, Option<&Config::verbosity, "-v", "--verbosity">>;
    const char* const attached[] = {"tool", "-v3"};
    CLST_ASSERT(Numeric::parse(2, attached, config));
    CLST_ASSERT_EQ(config.verbosity, 3);
    const char* const separated[] = {"tool", "--verbosity", "2"};
    CLST_ASSERT(Numeric::parse(3, separated, config));
    CLST_ASSERT_EQ(config.verbosity, 2);

    config.verbosity = 0;
    const char* const grouped[] = {"tool", "-vvv", "-jobs", "-5"};
    CLST_ASSERT(Args::parse(4, grouped, config));
    CLST_ASSERT_EQ(config.verbosity, 3);
    CLST_ASSERT_EQ(config.jobs, -5);

    constexpr auto max_count = std::numeric_limits<unsigned char>::max();
    using Counter = Parser<ByteCounter, CountFlag<&ByteCounter::count, max_count, "-v">>;
    ByteCounter counter;
    const char* const repetitions[] = {"tool", "-v", "-v", "-vvv"};
    CLST_ASSERT(Counter::parse(4, repetitions, counter));
    CLST_ASSERT_EQ(counter.count, max_count);
}

void test_text_bindings()
{
    Strings config;
    char owned[] = "--owned=0,2,3 with additional text";
    const char* const argv[] = {"tool", owned, "--borrowed=", "-p-named"};
    CLST_ASSERT(StringArgs::parse(4, argv, config));
    CLST_ASSERT_EQ(config.owned, "0,2,3 with additional text");
    owned[8] = 'x';
    CLST_ASSERT_EQ(config.owned.front(), '0');
    CLST_ASSERT(config.borrowed.empty());
    CLST_ASSERT_EQ(config.borrowed.data(), argv[2] + 11);
    CLST_ASSERT_EQ(config.pointer, argv[3] + 2);

    Config args;
    const char* const separated[] = {"tool", "--output", "--", "--force"};
    CLST_ASSERT(Args::parse(4, separated, args));
    CLST_ASSERT_EQ(args.path, separated[2]);
    CLST_ASSERT(args.force);
}

void test_positionals()
{
    Config config;
    const char* const argv[] = {"--jobs", "", "-vj", "-4", "file", "--jobs=6", "-", "--", "--output", "--"};
    std::array<const char*, 5> operands{};
    std::size_t count = 0;
    constexpr auto argc = static_cast<int>(std::size(argv));
    const auto result = Args::parse(argc, argv, config, [&](const char* operand) noexcept {
        if (count == operands.size()) {
            return false;
        }
        operands[count++] = operand;
        return true;
    });
    CLST_ASSERT(result);
    CLST_ASSERT_EQ(result.index, argc);
    CLST_ASSERT_EQ(config.verbosity, 1);
    CLST_ASSERT_EQ(config.jobs, 6);
    CLST_ASSERT_EQ(count, operands.size());
    const std::array expected{argv[1], argv[4], argv[6], argv[8], argv[9]};
    CLST_ASSERT_EQ(operands, expected);
}

template<std::size_t N>
void expect_error(Config& config, const char* const (&argv)[N], ParseError error, int index)
{
    const auto result = Args::parse(static_cast<int>(N), argv, config);
    CLST_ASSERT(!result);
    CLST_ASSERT_EQ(result.error, error);
    CLST_ASSERT_EQ(result.index, index);
}

void test_errors()
{
    Config config;
    expect_error(config, {"tool", "--jobs=4", "--jobs", "5tail", "--force"}, ParseError::invalid_value, 3);
    CLST_ASSERT_EQ(config.jobs, 4);
    CLST_ASSERT(!config.force);
    expect_error(config, {"tool", "--jobs=999999999999999999999999"}, ParseError::invalid_value, 1);
    expect_error(config, {"tool", "--jobs="}, ParseError::invalid_value, 1);
    expect_error(config, {"tool", "-j0,2,3"}, ParseError::invalid_value, 1);
    CLST_ASSERT_EQ(config.jobs, 4);
    expect_error(config, {"tool", "--jobs"}, ParseError::missing_value, 1);
    expect_error(config, {"tool", "-vj"}, ParseError::missing_value, 1);
    CLST_ASSERT_EQ(config.verbosity, 1);
    expect_error(config, {"tool", "--force="}, ParseError::unexpected_value, 1);
    CLST_ASSERT(!config.force);
    expect_error(config, {"tool", "--unknown=value"}, ParseError::unknown_option, 1);
    expect_error(config, {"tool", "-fx"}, ParseError::unknown_option, 1);
    CLST_ASSERT(config.force);
    expect_error(config, {"tool", "-"}, ParseError::unexpected_positional, 1);

    const char* const argv[] = {"tool", "--jobs=3", "file"};
    const auto rejected = Args::parse(3, argv, config, [](const char*) noexcept { return false; });
    CLST_ASSERT_EQ(rejected.error, ParseError::unexpected_positional);
    CLST_ASSERT_EQ(rejected.index, 2);
    CLST_ASSERT_EQ(config.jobs, 3);

    struct CallbackError {};
    CLST_ASSERT_THROW(Args::parse(3, argv, config, [](const char*) -> bool { throw CallbackError{}; }), CallbackError);
}

} // namespace

int argparse(int, char*[])
{
    test_option_values();
    test_text_bindings();
    test_positionals();
    test_errors();
    return 0;
}
