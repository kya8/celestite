#ifndef CLST_SYS_UTILS_HPP
#define CLST_SYS_UTILS_HPP

#include <string>
#include <filesystem>
#include <cstddef>
#include <cstdio>

//FIXME: Use SystemError exceptions

namespace clst {

std::string get_hostname();
std::filesystem::path get_proc_path();

struct MemoryInfo {
    std::size_t total; // total physical memory in bytes
    std::size_t available; // available physical memory
};

MemoryInfo get_mem_info();

bool is_tty(std::FILE* file) noexcept;

bool is_colorterm() noexcept;
bool is_colorterm(std::FILE* file) noexcept;

} // namespace clst

#endif // CLST_SYS_UTILS_HPP
