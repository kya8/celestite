#ifndef SLATE_CAMERA_INFO_HPP
#define SLATE_CAMERA_INFO_HPP

#include <string_view>
#include <string>
#include "varmap.hpp"
#include "slate/export.h"

namespace slate {

enum class CameraVendor {
    GoPro,
    Insta360,
    Unknown
};

SLATE_EXPORT [[nodiscard]] std::string_view to_string(CameraVendor type) noexcept;

struct CameraInfo {
    CameraVendor vendor = CameraVendor::Unknown;
    GroupedVarMap extras;

    SLATE_EXPORT [[nodiscard]] std::string describe() const noexcept;
};

} // namespace slate

#endif /* SLATE_CAMERA_INFO_HPP */
