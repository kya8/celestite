#ifndef CLST_COMMON_VERSION_HPP
#define CLST_COMMON_VERSION_HPP

namespace clst::version {

struct Version {
    const char* git_desc;
    const char* git_branch;
    const char* git_tag;
    int         git_version_major;
    int         git_version_minor;
    int         git_version_patch;
    const char* commit_date;
    const char* commit_hash;
    const char* target_os;
    const char* target_arch;
    const char* build_type;
    const char* compiler_name;
    const char* compiler_version;
    const char* host_os;
    const char* host_hostname;
};

const Version& get() noexcept;
void print_info() noexcept;

} // namespace clst::version

#endif /* CLST_COMMON_VERSION_HPP */
