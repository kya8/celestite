#include <clst/sys_dirs.hpp>
#include <iostream>

int sys_dirs(int, char*[])
{
    std::cout << "Config dir: " << clst::config_dir() << '\n';
    std::cout << "Cache dir: " << clst::cache_dir() << '\n';
    std::cout << "Data dir: " << clst::data_dir() << '\n';
    std::cout << "State dir: " << clst::state_dir() << '\n';
    std::cout << "Tmp dir: " << clst::tmp_dir() << '\n';
    std::cout << "Home dir: " << clst::home_dir() << '\n';
    return 0;
}
