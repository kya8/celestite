#ifndef CLST_COMMON_FS_HPP
#define CLST_COMMON_FS_HPP

#include <filesystem>
#include <vector>
#include <concepts>
#include <type_traits>

namespace util {

namespace fs = std::filesystem;

std::vector<fs::path> get_files(const fs::path& dir, const fs::path& ext);

fs::path find_most_occuring_extension(const fs::path& dir);

std::vector<fs::path> get_most_occuring_extension_files(const fs::path& dir);

namespace detail {

inline constexpr bool native_path_is_string = std::same_as<fs::path::string_type, std::string>;

} // namespace detail

/**
 * Get a native string reference if possible.
 * @param[in] p An lvalue fs::path.
 * @note We don't want to return a reference to a temporary object.
 *       The `requires` constraint disables the overload for rvalue arguments.
 */
inline constexpr auto to_str = []<typename Path>
requires (std::is_lvalue_reference_v<Path> && std::same_as<std::remove_cvref_t<Path>, fs::path>)
(Path&& p) noexcept(detail::native_path_is_string) -> decltype(auto) {
    if constexpr (detail::native_path_is_string) {
        return p.native();
    } else {
        return p.string();
    }
};

// Allows applying callable objects with pipe operator.
template <typename T, typename F>
    requires std::invocable<F&&, T&&>
decltype(auto) operator|(T&& lhs, F&& func) {
    return std::forward<F>(func)(std::forward<T>(lhs));
}

} //namespace util

#endif /* CLST_COMMON_FS_HPP */
