#ifndef CLST_DETECT_HPP
#define CLST_DETECT_HPP

#include "camera_info.hpp"
#include <optional>
#include "clst/export.h"

namespace clst {

CLST_EXPORT [[nodiscard]] std::optional<CameraInfo> detect(const char* video_file, bool metadata_only = false) noexcept;

} // namespace clst


#endif /* CLST_DETECT_HPP */
