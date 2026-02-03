#ifndef CLST_SYS_DIRS_HPP
#define CLST_SYS_DIRS_HPP

#include <filesystem>

namespace clst {

namespace fs = std::filesystem;

fs::path config_dir();
fs::path cache_dir();
fs::path state_dir();
fs::path data_dir();
fs::path tmp_dir();
fs::path home_dir();

} // namespace sys


#endif /* CLST_SYS_DIRS_HPP */
