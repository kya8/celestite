#include "clst/sys_utils.hpp"
#include "clst/error.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h> // _isatty
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#define IS_POSIX
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <unistd.h>
#include <limits.h>
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#endif
#else
    #error "Could not detect supported platform"
#endif

#ifdef __linux__
#include <fstream> // for parsing /proc/meminfo
#include <cstdio>  // sscanf
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <mach-o/dyld.h>   // _NSGetExecutablePath
#include <sys/syslimits.h> // PATH_MAX
#include <sys/sysctl.h>
#include <mach/mach.h>
#endif

#include <cstdlib> // std::getenv
#include <string_view>

namespace clst {

std::string
get_hostname()
{
#ifdef _WIN32
    char buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD sz = std::size(buf);
    if (GetComputerNameA(buf, &sz) == 0)
        SystemError::throw_last();
    return buf;
#elif defined(IS_POSIX)
    char buf[HOST_NAME_MAX + 2] {0};
    if (gethostname(buf, std::size(buf) - 1) == 0)
        return buf;
    SystemError::throw_last();
#endif
}

std::filesystem::path
get_proc_path()
{
#if defined(_WIN32)
    wchar_t buf[512];
    const auto ret = GetModuleFileNameW(NULL, buf, 512);
    if (ret == 512 || ret == 0) {
        SystemError::throw_last();
    }
    return std::filesystem::path(buf);
#elif defined(__linux__)
    return std::filesystem::canonical("/proc/self/exe");
#elif defined(__APPLE__) && defined(__MACH__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) {
        // in line with glibc
        throw SystemError(std::errc::filename_too_long);
    }
    return std::filesystem::canonical(buf);
#else // not supported
#endif
}

MemoryInfo
get_mem_info()
{
#if defined(_WIN32)
    MEMORYSTATUSEX mem_info;
    mem_info.dwLength = sizeof(mem_info);
    if (!GlobalMemoryStatusEx(&mem_info)) {
        SystemError::throw_last();
    }
    return MemoryInfo{mem_info.ullTotalPhys, mem_info.ullAvailPhys};
#elif defined(__linux__)
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo) {
        throw RuntimeError("Cannot open /proc/meminfo");
    }
    unsigned long total = 0, avail = 0;
    std::string line;
    while ((total == 0 || avail == 0) && std::getline(meminfo, line)) {
        if (total == 0 && std::sscanf(line.c_str(), "MemTotal: %lu kB", &total) == 1)
            continue;
        if (avail == 0 && std::sscanf(line.c_str(), "MemAvailable: %lu kB", &avail) == 1)
            continue;
    }
    if (total == 0 || avail == 0) {
        throw RuntimeError("Error parsing /proc/meminfo");
    }
    return MemoryInfo{total * 1024, avail * 1024};
#elif defined(__APPLE__) && defined(__MACH__)
    uint64_t memsize = 0;
    size_t len = sizeof(memsize);
    if (sysctlbyname("hw.memsize", &memsize, &len, nullptr, 0) != 0) {
        SystemError::throw_last();
    }

    const auto host = mach_host_self();
    vm_size_t page_size;
    if (host_page_size(host, &page_size) != KERN_SUCCESS) {
        throw RuntimeError("mach internal error"); // We don't want to define an ecat for kern_return_t
    }
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vm_stat), &count) != KERN_SUCCESS) {
        throw RuntimeError("mach internal error");
    }
    return MemoryInfo{memsize, page_size * (vm_stat.free_count + vm_stat.inactive_count)};
#else // not supported
#endif
}

bool
is_tty(std::FILE* file) noexcept
{
#ifdef _WIN32
    return _isatty(_fileno(file));
#else
    return isatty(fileno(file));
#endif
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // std::getenv
#endif
bool is_colorterm() noexcept
{
    const auto to_sv = [](const char* s) { // Since constructing string_view from null char* is UB.
        return s? std::string_view{s} : std::string_view{};
    };
    const std::string_view term = to_sv(std::getenv("TERM"));
    const std::string_view colorterm = to_sv(std::getenv("COLORTERM"));
    const auto contains = [&](std::string_view s) {
        return term.find(s) != std::string_view::npos || colorterm.find(s) != std::string_view::npos;
    };

    return colorterm == "1" ||
    // FIXME: Use GetConsoleMode on Win32
    // Ref: https://github.com/microsoft/terminal/issues/1040
           std::getenv("WT_SESSION") ||
           contains("256") ||
           contains("24bit") ||
           contains("truecolor");
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

bool is_colorterm(std::FILE* file) noexcept
{
    return is_tty(file) && is_colorterm();
}

} // namespace clst
