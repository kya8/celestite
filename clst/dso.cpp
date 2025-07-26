#include "clst/dso.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else // POSIX
#include <dlfcn.h>
#endif

namespace {

void* load_dso(const char* name) noexcept
{
#ifdef _WIN32
    return LoadLibraryExA(name, nullptr, 0);
#else
    return dlopen(name, RTLD_NOW);
#endif
}

}

namespace clst {

Dso::Dso(const char* name) noexcept : handle_(load_dso(name)) {}

Dso::~Dso() noexcept
{
    close();
}

Dso::Dso(Dso&& rhs) noexcept
{
    handle_ = rhs.handle_;
    rhs.handle_ = nullptr;
}

Dso& Dso::operator=(Dso&& rhs) noexcept
{
    close();
    handle_ = rhs.handle_;
    rhs.handle_ = nullptr;

    return *this;
}

bool Dso::open(const char* name) noexcept
{
    if (handle_) {
        return false;
    }
    handle_ = load_dso(name);
    return handle_ != nullptr;
}

bool Dso::is_open() noexcept
{
    return handle_ != nullptr;
}

bool Dso::close() noexcept
{
    if (!handle_) {
        return true;
    }
#ifdef _WIN32
    return FreeLibrary(static_cast<HMODULE>(handle_)) != 0;
#else
    return dlclose(handle_) == 0;
#endif
}

void* Dso::get_sym(const char* name) const noexcept
{
    if (!handle_) {
        return nullptr;
    }
#ifdef _WIN32
    return GetProcAddress(static_cast<HMODULE>(handle_), name);
#else
    // dlsym can return null as a valid address, but we treat it as an error.
    return dlsym(handle_, name);
#endif
}

} // namespace clst
