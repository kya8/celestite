#include "common/fixed_string.hpp"
#include "test.h"

#include <concepts>
#include <cstdint>
#include <limits>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace {

using clst::FixedString;
using clst::to_fixed_string;
using namespace std::string_view_literals;

template<FixedString Text>
constexpr auto static_view = Text.view();

static_assert(static_view<"abc"> == "abc");
static_assert(static_view<"">.empty());

constexpr FixedString text{"abc"};
static_assert(std::same_as<decltype(text), const FixedString<3>>);
static_assert(std::ranges::contiguous_range<FixedString<3>>);
static_assert(std::ranges::contiguous_range<const FixedString<3>>);
static_assert(text.size() == 3 && text.capacity() == 3);
static_assert(text.begin() == text.cbegin() && text.end() == text.cend());
static_assert(text.cend() == text.data() + text.size());
static_assert(text.c_str()[text.size()] == '\0');
static_assert(text[1] == text.at(1));

constexpr char bytes[] = {'x', '\0', 'z'};
constexpr FixedString<3> copied{bytes};
static_assert(copied.view() == "x\0z"sv && copied.c_str()[3] == '\0');
static_assert(FixedString<0>{nullptr}.c_str()[0] == '\0');

template<std::size_t Begin, std::size_t End>
concept ValidSlice = requires { text.substr<Begin, End>(); };

template<class T>
concept CanPop = requires(const T& value) { value.pop_back(); value.pop_front(); };

static_assert(text.substr<1, 2>().view() == "b");
static_assert(text.substr<1>().view() == "bc");
static_assert(text.substr<3, 3>().c_str()[0] == '\0');
static_assert(!ValidSlice<2, 1> && !ValidSlice<0, 4>);
static_assert(!CanPop<FixedString<0>>);

static_assert(std::same_as<decltype(text + FixedString{"de"}), FixedString<5>>);
static_assert((text + FixedString{"de"}).view() == "abcde");
static_assert((text + "de").view() == "abcde" && ("ab" + FixedString{"c"}).view() == "abc");
static_assert(text.push_back('d').view() == "abcd");
static_assert(text.push_front('z').view() == "zabc");
static_assert(text.pop_back().view() == "ab" && text.pop_front().view() == "bc");
static_assert(FixedString{"a"}.pop_back().size() == 0);
static_assert(text.resize<2>().view() == "ab");
static_assert(text.resize<3>().view() == "abc");
static_assert(text.resize<5>().view() == "abc\0\0"sv);
static_assert(text.resize<5>('-').view() == "abc--" && text.resize<5>('-').c_str()[5] == '\0');
static_assert(text.append<2>(bytes).view() == "abcx\0"sv);
static_assert(text.append<0>(static_cast<const char*>(nullptr)).view() == "abc");

static_assert(to_fixed_string<0>().view() == "0");
static_assert(to_fixed_string<-42>().view() == "-42");
static_assert(to_fixed_string<123u>().view() == "123");
static_assert(to_fixed_string<std::numeric_limits<std::int64_t>::min()>().view() == "-9223372036854775808");
constexpr auto u64_max = to_fixed_string<std::numeric_limits<std::uint64_t>::max()>();
static_assert(u64_max.view() == "18446744073709551615" && u64_max.c_str()[u64_max.size()] == '\0');
static_assert(static_view<to_fixed_string<-7>()> == "-7");

void test_access()
{
    char source[] = {'a', 'b', 'c'};
    FixedString<3> owned{source};
    source[0] = 'z';
    owned.at(1) = 'B';
    owned[2] = 'C';
    const auto& const_owned = owned;
    CLST_ASSERT_EQ(const_owned.at(0), 'a');
    CLST_ASSERT_EQ(const_owned.view(), "aBC");
    CLST_ASSERT_THROW(owned.at(owned.size()), std::out_of_range);
    CLST_ASSERT_THROW(const_owned.at(const_owned.size()), std::out_of_range);
}

void test_input_iterator()
{
    std::istringstream input{"xyz"};
    std::ranges::istream_view<char> chars{input};
    const auto result = text.append<2>(chars.begin());
    CLST_ASSERT_EQ(result.view(), "abcxy");
    CLST_ASSERT_EQ(result.c_str()[result.size()], '\0');
    CLST_ASSERT_EQ(input.get(), 'z');
}

} // namespace

int fixed_string(int, char*[])
{
    test_access();
    test_input_iterator();
    return 0;
}
