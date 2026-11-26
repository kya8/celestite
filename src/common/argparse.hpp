#ifndef CLST_COMMON_ARGPARSE_HPP
#define CLST_COMMON_ARGPARSE_HPP

#include "fixed_string.hpp"

#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace clst::cli {

enum class ParseStatus {
    ok,
    unknown_option,
    missing_value,
    invalid_value,
    unexpected_value,
    unexpected_positional,
};

struct [[nodiscard]] ParseResult {
    ParseStatus error;
    // argc on ordinary completion, or the argv index of the error or stop flag.
    int index;

    constexpr explicit operator bool() const noexcept { return error == ParseStatus::ok; }
};

namespace detail {

enum class BindingKind { flag, stop, count, value };
enum class AliasForm { long_option, short_option, short_alias };

consteval AliasForm alias_form(std::string_view name) noexcept
{
    if (name.starts_with("--")) {
        return AliasForm::long_option;
    }
    return name.size() == 2 ? AliasForm::short_option : AliasForm::short_alias;
}

template<FixedString Alias>
inline constexpr std::string_view alias_view = Alias.view();

template<AliasForm Form, FixedString Alias>
constexpr bool matches_alias(auto key) noexcept
{
    constexpr auto name = alias_view<Alias>;
    if constexpr (alias_form(name) != Form) {
        return false;
    } else if constexpr (Form == AliasForm::short_option) {
        return key == name[1];
    } else {
        return key == name;
    }
}

template<BindingKind Kind, auto Member, auto Limit, FixedString... Aliases>
struct Binding {
    static constexpr auto kind = Kind;
    static constexpr auto member = Member;
    static constexpr auto limit = Limit;
    static constexpr std::array<std::string_view, sizeof...(Aliases)> aliases{Aliases.view()...};

    template<AliasForm Form>
    static constexpr bool has_alias = ((alias_form(Aliases.view()) == Form) || ...);

    template<AliasForm Form>
    static constexpr auto match(auto key) noexcept
    {
        if constexpr (Kind == BindingKind::stop) {
            const std::string_view* matched = nullptr;
            const auto try_alias = [&]<FixedString Alias> {
                if (!matches_alias<Form, Alias>(key)) {
                    return false;
                }
                matched = &alias_view<Alias>;
                return true;
            };
            (void)(try_alias.template operator()<Aliases>() || ...);
            return matched;
        } else {
            return (matches_alias<Form, Aliases>(key) || ...);
        }
    }
};

template<class T>
inline constexpr bool is_binding = false;

template<BindingKind Kind, auto Member, auto Limit, FixedString... Aliases>
inline constexpr bool is_binding<Binding<Kind, Member, Limit, Aliases...>> = true;

constexpr bool valid_alias(std::string_view name) noexcept
{
    if (name.size() < 2 || name.front() != '-' || name == "--") {
        return false;
    }
    for (const char ch : name) {
        if (ch == '=' || ch == '\0' || ch == ' ' || (ch >= '\t' && ch <= '\r')) {
            return false;
        }
    }
    return true;
}

template<class... Bindings>
consteval bool valid_aliases()
{
    std::array<std::string_view, (std::size_t{} + ... + Bindings::aliases.size())> seen{};
    std::size_t count = 0;
    const auto add_aliases = [&](const auto& aliases) {
        if (aliases.empty()) {
            return false;
        }
        for (const auto alias : aliases) {
            if (!valid_alias(alias)) {
                return false;
            }
            for (std::size_t i = 0; i < count; ++i) {
                if (alias == seen[i]) {
                    return false;
                }
            }
            seen[count++] = alias;
        }
        return true;
    };
    return (add_aliases(Bindings::aliases) && ...);
}

template<class T>
concept Integer = std::integral<T> && !std::same_as<T, bool> && requires (const char* ptr, T& value) {
    std::from_chars(ptr, ptr, value, 10);
};

template<class T>
concept Text = std::same_as<T, std::string> || std::same_as<T, std::string_view> || std::same_as<T, const char*>;

template<class Config, class Binding>
consteval bool valid_member()
{
    if constexpr (!std::is_member_object_pointer_v<decltype(Binding::member)> ||
                  !requires (Config& config) { config.*Binding::member; }) {
        return false;
    } else {
        using Field = std::remove_reference_t<decltype(std::declval<Config&>().*Binding::member)>;
        if constexpr (Binding::member == nullptr || std::is_const_v<Field> || std::is_volatile_v<Field>) {
            return false;
        } else if constexpr (Binding::kind == BindingKind::flag || Binding::kind == BindingKind::stop) {
            return std::same_as<Field, bool>;
        } else if constexpr (Binding::kind == BindingKind::count) {
            return Integer<Field> && std::integral<decltype(Binding::limit)> && requires { Field{Binding::limit}; };
        } else {
            return Integer<Field> || Text<Field>;
        }
    }
}

template<class T>
constexpr auto text_value(const char* value) noexcept
{
    if constexpr (std::same_as<T, std::string_view>) {
        return std::string_view{value, std::char_traits<char>::length(value)};
    } else {
        return value;
    }
}

template<Text T>
constexpr bool assign_value(T& field, const char* value) noexcept(noexcept(field = text_value<T>(value)))
{
    field = text_value<T>(value);
    return true;
}

template<Integer T>
constexpr bool assign_value(T& field, const char* value) noexcept
{
    const char* end = value + std::char_traits<char>::length(value);
    T parsed{};
    const auto result = std::from_chars(value, end, parsed, 10);
    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }
    field = parsed;
    return true;
}

template<class Config, class Binding>
consteval bool nothrow_binding()
{
    if constexpr (Binding::kind == BindingKind::value) {
        return noexcept(assign_value(std::declval<Config&>().*Binding::member, std::declval<const char*>()));
    }
    return true;
}

struct Arguments {
    int argc;
    const char* const* argv;
    int index = 1;
};

} // namespace detail

// Set a Boolean member to true on each occurrence.
template<auto Member, FixedString... Aliases>
using Flag = detail::Binding<detail::BindingKind::flag, Member, 0, Aliases...>;

// Set a Boolean member to true and return a successful early exit.
template<auto Member, FixedString... Aliases>
using StopFlag = detail::Binding<detail::BindingKind::stop, Member, 0, Aliases...>;

// Increment an integer member whenever it is below Limit.
template<auto Member, auto Limit, FixedString... Aliases>
using CountFlag = detail::Binding<detail::BindingKind::count, Member, Limit, Aliases...>;

// Assign text or parse a complete base-10 integer into the member.
template<auto Member, FixedString... Aliases>
using Option = detail::Binding<detail::BindingKind::value, Member, 0, Aliases...>;

/**
 * Bind command-line options to members of a caller-owned configuration.
 * @code
 * struct Config { int verbosity = 0; int jobs = 1; };
 * using Args = clst::cli::Parser<Config,
 *     clst::cli::CountFlag<&Config::verbosity, 3, "-v", "--verbose">,
 *     clst::cli::Option<&Config::jobs, "-j", "--jobs">>;
 * Config config;
 * const auto result = Args::parse(argc, argv, config);
 * @endcode
 * parse takes a nonnegative argc and valid NUL-terminated strings at indices
 * [1, argc). It skips argv[0]. A positional callable receives each operand as
 * a const char* and returns true to accept it. The overload without a callable
 * rejects operands.
 *
 * A standalone -- makes the remaining tokens operands. Exact aliases take
 * precedence over short-option groups. A value option within a group consumes
 * its suffix or the next token. Long values can follow an equals sign or occupy
 * a separate token. A separate value can start with a dash.
 * A StopFlag ends parsing immediately, including within a short-option group.
 * With a StopFlag, Result provides is_early_exit() and early_exit_opt().
 *
 * String views and pointers borrow the corresponding argv string. String
 * assignments and positional calls use their usual allocation and exception
 * behavior. Successful assignments and callbacks take effect as arguments
 * are processed, including when a later argument fails.
 */
template<class Config, class... Bindings>
requires ((detail::is_binding<Bindings> && ...) && (detail::valid_member<Config, Bindings>() && ...) &&
          detail::valid_aliases<Bindings...>())
class Parser {
    using AliasForm = detail::AliasForm;
    using BindingKind = detail::BindingKind;

    static constexpr bool nothrow_bindings = (detail::nothrow_binding<Config, Bindings>() && ...);
    static constexpr bool has_stop_flag = ((Bindings::kind == BindingKind::stop) || ...);

    class [[nodiscard]] EarlyExitResult : public ParseResult {
    public:
        [[nodiscard]] constexpr bool is_early_exit() const noexcept requires has_stop_flag
        {
            return early_exit_ != nullptr;
        }

        // The matched alias has static storage. Other results yield a default view.
        [[nodiscard]] constexpr std::string_view early_exit_opt() const noexcept requires has_stop_flag
        {
            return early_exit_ ? *early_exit_ : std::string_view{};
        }

    private:
        friend class Parser;

        constexpr EarlyExitResult(ParseStatus error, int index, const std::string_view* early_exit = nullptr) noexcept
            : ParseResult{error, index}, early_exit_(early_exit) {}

        const std::string_view* early_exit_;
    };

public:
    using Result = std::conditional_t<has_stop_flag, EarlyExitResult, ParseResult>;

    static constexpr Result parse(int argc, const char* const* argv, Config& config) noexcept(nothrow_bindings)
    {
        return parse(argc, argv, config, [](const char*) noexcept { return false; });
    }

    template<class Positional>
    requires requires (Positional& handler, const char* const& operand) {
        { handler(operand) } -> std::same_as<bool>;
    }
    static constexpr Result parse(int argc, const char* const* argv, Config& config, Positional&& on_positional)
        noexcept(nothrow_bindings && noexcept(on_positional(std::declval<const char* const&>())))
    {
        detail::Arguments args{argc, argv};
        bool positional_only = false;
        for (; args.index < argc; ++args.index) {
            const char* const token = argv[args.index];
            if (positional_only || token[0] != '-' || token[1] == '\0') {
                if (!on_positional(token)) {
                    return {ParseStatus::unexpected_positional, args.index};
                }
                continue;
            }

            if (token[1] == '-') {
                if (token[2] == '\0') {
                    positional_only = true;
                    continue;
                }
                const char* end = token + 2;
                while (*end != '\0' && *end != '=') {
                    ++end;
                }
                const char* suffix = *end == '=' ? end + 1 : nullptr;
                const std::string_view name{token, static_cast<std::size_t>(end - token)};
                if (const auto result = consume<AliasForm::long_option>(name, suffix, args, config);
                    should_return<AliasForm::long_option>(result)) {
                    return result;
                }
                continue;
            }

            if constexpr ((Bindings::template has_alias<AliasForm::short_alias> || ...)) {
                if (token[2] != '\0') {
                    const char* suffix = nullptr;
                    const auto name = detail::text_value<std::string_view>(token);
                    const auto result = consume<AliasForm::short_alias>(name, suffix, args, config);
                    if (result.error != ParseStatus::unknown_option) {
                        if (should_return<AliasForm::short_alias>(result)) {
                            return result;
                        }
                        continue;
                    }
                }
            }

            const char* suffix = token + 1;
            while (suffix && *suffix != '\0') {
                const char name = *suffix++;
                if (const auto result = consume<AliasForm::short_option>(name, suffix, args, config);
                    should_return<AliasForm::short_option>(result)) {
                    return result;
                }
            }
        }
        return {ParseStatus::ok, argc};
    }

private:
    template<AliasForm Form>
    static constexpr bool should_return(const Result& result) noexcept
    {
        if constexpr (((Bindings::kind == BindingKind::stop && Bindings::template has_alias<Form>) || ...)) {
            return !result || result.is_early_exit();
        } else {
            return !result;
        }
    }

    template<AliasForm Form>
    static constexpr Result consume(auto name, const char*& suffix, detail::Arguments& args, Config& config) noexcept(nothrow_bindings)
    {
        auto error = ParseStatus::unknown_option;
        const std::string_view* early_exit = nullptr;
        const auto try_binding = [&]<class Binding> {
            if constexpr (Binding::template has_alias<Form>) {
                if (const auto match = Binding::template match<Form>(name)) {
                    error = apply<Form, Binding>(suffix, args, config);
                    if constexpr (Binding::kind == BindingKind::stop) {
                        if (error == ParseStatus::ok) {
                            early_exit = match;
                        }
                    }
                    return true;
                }
            }
            return false;
        };
        (void)(try_binding.template operator()<Bindings>() || ...);
        if constexpr (has_stop_flag) {
            return {error, args.index, early_exit};
        } else {
            return {error, args.index};
        }
    }

    template<AliasForm Form, class Binding>
    static constexpr ParseStatus apply(const char*& suffix, detail::Arguments& args, Config& config)
        noexcept(detail::nothrow_binding<Config, Binding>())
    {
        auto& field = config.*Binding::member;
        if constexpr (Binding::kind == BindingKind::value) {
            const char* value = suffix;
            if constexpr (Form == AliasForm::short_option) {
                if (*suffix == '\0') {
                    value = nullptr;
                }
                // A value-taking short option ends the group.
                suffix = nullptr;
            }
            if (!value) {
                if (args.index + 1 == args.argc) {
                    return ParseStatus::missing_value;
                }
                value = args.argv[++args.index];
            }
            if (!detail::assign_value(field, value)) {
                return ParseStatus::invalid_value;
            }
        } else {
            if constexpr (Form == AliasForm::long_option) {
                if (suffix) {
                    return ParseStatus::unexpected_value;
                }
            }
            if constexpr (Binding::kind == BindingKind::count) {
                constexpr auto limit = std::remove_reference_t<decltype(field)>{Binding::limit};
                if (field < limit) {
                    ++field;
                }
            } else {
                field = true;
            }
        }
        return ParseStatus::ok;
    }
};

} // namespace clst::cli

#endif // CLST_COMMON_ARGPARSE_HPP
