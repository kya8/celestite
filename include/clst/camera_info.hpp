#ifndef CLST_CAMERA_INFO_HPP
#define CLST_CAMERA_INFO_HPP

#include <string_view>
#include <string>
#include "varmap.hpp"
#include "clst/export.h"

namespace clst {

enum class CameraVendor {
    GoPro,
    Insta360,
    Unknown
};

CLST_EXPORT [[nodiscard]] std::string_view to_string(CameraVendor type) noexcept;

struct CameraInfo {
    CameraVendor vendor = CameraVendor::Unknown;
    GroupedVarMap extras;

    CLST_EXPORT [[nodiscard]] std::string describe() const noexcept;
};

} // namespace clst

#endif /* CLST_CAMERA_INFO_HPP */
