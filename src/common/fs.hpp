#ifndef SLATE_COMMON_FS_HPP
#define SLATE_COMMON_FS_HPP

#include <filesystem>
#include <vector>

namespace util {

std::vector<std::filesystem::path> get_files(const std::filesystem::path& dir, const std::filesystem::path& ext);

std::filesystem::path find_most_occuring_extension(const std::filesystem::path& dir);

std::vector<std::filesystem::path> get_most_occuring_extension_files(const std::filesystem::path& dir);

} //namespace util

#endif /* SLATE_COMMON_FS_HPP */
