#ifndef CLST_COMMON_TIME_UTILS_HPP
#define CLST_COMMON_TIME_UTILS_HPP

#include <ctime>
#include <string>

namespace util {

/**
 * @brief Safe localtime function.
 * @return 0 on success, non-zero on failure.
 */
int localtime(const std::time_t& t, std::tm& tm) noexcept;

int gmtime(const std::time_t& t, std::tm& tm) noexcept;

std::string format_time(const std::tm& tm, const char* fmt);

} // namespace util

#endif /* CLST_COMMON_TIME_UTILS_HPP */
