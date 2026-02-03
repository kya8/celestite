#include "clst/sys_dirs.hpp"

#ifdef _WIN32
#  define IS_WIN32 1
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#  include <Shlobj.h> // SHGetKnownFolderPath
#  include <comdef.h>
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#  define IS_POSIX 1
#  ifndef _POSIX_C_SOURCE
#    define _POSIX_C_SOURCE 200809L
#  endif
#  include <unistd.h>
#  include <limits.h>
#  include <stdlib.h>
#  include <sys/types.h>
#  include <pwd.h>
#  include <memory>
#  include <errno.h>
#  ifdef __linux__
#    define IS_LINUX 1
#  elif defined(__APPLE__) && defined(__MACH__)
#    define IS_MACOS 1
#    ifndef _DARWIN_C_SOURCE
#      define _DARWIN_C_SOURCE 1
#    endif
#    include <sysdir.h> // macOS 10.12+
#  endif
#else
#  error "Could not detect supported platform"
#endif

#include <system_error>
#include "clst/scope_guard.hpp"

namespace clst {

#if defined(IS_WIN32)

namespace {

fs::path get_known_folder(REFKNOWNFOLDERID id)
{
    wchar_t* s;
    const auto ret = SHGetKnownFolderPath(id, 0, NULL, &s);
    ScopeGuard sg{[&] { CoTaskMemFree(s); }};
    // COM error codes aren't for system_category.
    // Ideally, we should derive a custom com_category to wrap HRESULT into error_code.
    // For now, throw a runtime_error with the error msg of the HRESULT.
    if (ret != S_OK) {
        throw std::runtime_error(_com_error(ret).ErrorMessage());
    }
    return fs::path{s}; // The scope guard will run after return;
}

} // namespace

fs::path config_dir()
{
    return get_known_folder(FOLDERID_RoamingAppData);
}

fs::path cache_dir()
{
    return get_known_folder(FOLDERID_LocalAppData);
}

fs::path state_dir()
{
    return get_known_folder(FOLDERID_LocalAppData);
}

fs::path data_dir()
{
    return get_known_folder(FOLDERID_LocalAppData);
}

fs::path tmp_dir()
{
    std::wstring buf(MAX_PATH + 1, 0); // resize_for_overwrite C++23
    // +1 is for well-defined behavior before LWG2475
    const auto ret = GetTempPath2W(MAX_PATH + 1, buf.data());
    if (ret > MAX_PATH + 1) {
        // this cannot happen
        throw std::runtime_error("Path too long");
    }
    if (ret == 0) {
        throw std::system_error(GetLastError(), std::system_category());
    }
    buf.resize(ret);
    return std::move(buf);
}

#elif defined(IS_MACOS)

namespace {

fs::path get_sysdir(sysdir_search_path_directory_t dir)
{
    char path[PATH_MAX]{0};
    auto state = sysdir_start_search_path_enumeration(dir, SYSDIR_DOMAIN_MASK_USER);
    if (sysdir_get_next_search_path_enumeration(state, path) == 0) {
        throw std::runtime_error("sysdir not found");
    }
    // path will contain `~' for user's home directory, we expand it manually
    if (path[0] == '~') { // '~' can only be at the beginning of the path.
        auto ret = home_dir();
        ret += (path + 1);
        return ret;
    }
    return path;
}

} // namespace

fs::path config_dir()
{
    return get_sysdir(SYSDIR_DIRECTORY_APPLICATION_SUPPORT);
}

fs::path cache_dir()
{
    return get_sysdir(SYSDIR_DIRECTORY_CACHES);
}

fs::path data_dir()
{
    return get_sysdir(SYSDIR_DIRECTORY_APPLICATION_SUPPORT);
}

fs::path state_dir()
{
    return get_sysdir(SYSDIR_DIRECTORY_APPLICATION_SUPPORT);
}

fs::path tmp_dir()
{
    errno = 0;
    auto bufsize = confstr(_CS_DARWIN_USER_TEMP_DIR, NULL, 0);
    if (bufsize == 0) {
        if (errno) throw std::system_error(errno, std::system_category());
        throw std::runtime_error("confstr: value not defined");
    }

    const auto buf = std::make_unique<char[]>(bufsize);
    errno = 0;
    bufsize = confstr(_CS_DARWIN_USER_TEMP_DIR, buf.get(), bufsize);
    if (bufsize == 0) {
        if (errno) throw std::system_error(errno, std::system_category());
        throw std::runtime_error("confstr: value not defined");
    }
    return buf.get();
}

#elif defined(IS_POSIX)
// Unices that aren't macOS, e.g. GNU/Linux & BSDs.
// Use XDG Base dirs: https://specifications.freedesktop.org/basedir/latest/

namespace {

fs::path get_dir(const char* env, fs::path fallback)
{
    const auto env_val = getenv(env);
    const fs::path dir = env_val ? env_val : std::move(fallback);
    if (!fs::is_directory(dir)) {
        fs::create_directories(dir);
    }
    return dir;
}

} // namespace

fs::path config_dir()
{
    return get_dir("XDG_CONFIG_HOME", home_dir() / ".config");
}

fs::path cache_dir()
{
    return get_dir("XDG_CACHE_HOME", home_dir() / ".cache");
}

fs::path data_dir()
{
    return get_dir("XDG_DATA_HOME", home_dir() / ".local/share");
}

fs::path state_dir()
{
    return get_dir("XDG_STATE_HOME", home_dir() / ".local/share");
}

fs::path tmp_dir()
{
    const auto env = getenv("TMPDIR");
    if (env) {
        return env;
    }
    return "/tmp";
}

#endif

fs::path home_dir()
{
#ifdef IS_WIN32
    return get_known_folder(FOLDERID_Profile);
#else
    // Check $HOME first
    const auto env = getenv("HOME");
    if (env && env[0]) {
        return env;
    }
    struct passwd pwd;
    struct passwd* result;
    static constexpr auto bufsize = 4096; // Should be more than enough. Or, sysconf(_SC_GETPW_R_SIZE_MAX)
    const auto buf = std::make_unique<char[]>(bufsize);
    const auto ret = getpwuid_r(getuid(), &pwd, buf.get(), bufsize, &result);
    if (result == NULL) {
        if (ret == 0) {
            throw std::runtime_error("User entry not found");
        } 
        throw std::system_error(ret, std::system_category());
    }
    return pwd.pw_dir;
#endif
}

} // namespace sys
