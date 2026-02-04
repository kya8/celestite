#ifndef CLST_SYS_DIRS_HPP
#define CLST_SYS_DIRS_HPP

#include <filesystem>

namespace clst {

namespace fs = std::filesystem;

/*
 * Get user-specific base directory for configuration files.
 * 
 * @note On platforms where XDG Base Directory Specification is used,
 *       the directory is created before return.
 */
fs::path config_dir();

/*
 * Get user-specific base directory for cache.
 * 
 * @note On platforms where XDG Base Directory Specification is used,
 *       the directory is created before return.
 */
fs::path cache_dir();

/*
 * Get user-specific base directory for state data.
 * 
 * @note On platforms where XDG Base Directory Specification is used,
 *       the directory is created before return.
 */
fs::path state_dir();

/*
 * Get user-specific base directory for data files.
 * 
 * @note On platforms where XDG Base Directory Specification is used,
 *       the directory is created before return.
 */
fs::path data_dir();

/*
 * Get user-specific base directory for temporary files.
 */
fs::path tmp_dir();

/*
 * Get the home directory of the current user.
 */
fs::path home_dir();

namespace xdg { // These variants return standard XDG directories regardless of the platform.

fs::path config_dir();
fs::path cache_dir();
fs::path state_dir();
fs::path data_dir();

} // namespace xdg

} // namespace sys


#endif /* CLST_SYS_DIRS_HPP */
