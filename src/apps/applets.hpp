#ifndef CLST_APPS_APPLETS_HPP
#define CLST_APPS_APPLETS_HPP

#include <string_view>

namespace clst {

struct Applet {
    using Fn = int(int, char**);
    std::string_view name;
    Fn& func;
    // std::string_view desc{};
};

int main_clst(int argc, char** argv) noexcept;
int main_calibrator(int argc, char** argv) noexcept;
int main_insta360_normalize(int argc, char** argv) noexcept;
int main_mp4join(int argc, char** argv) noexcept;

inline constexpr Applet applets[] {
    { "inspect"           , main_clst              },
    { "mp4join"           , main_mp4join            },
    { "insta360_normalize", main_insta360_normalize },
    { "calibrator"        , main_calibrator         },
};

// Global application verbosity. Use with care!
// Set it at application start.
inline int g_verbosity = 0;

} // namespace clst

#endif /* CLST_APPS_APPLETS_HPP */
