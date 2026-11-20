#ifndef SLATE_DETECT_HPP
#define SLATE_DETECT_HPP

#include "camera_info.hpp"
#include <optional>
#include "slate/export.h"

namespace slate {

SLATE_EXPORT [[nodiscard]] std::optional<CameraInfo> detect(const char* video_file, bool metadata_only = false) noexcept;

} // namespace slate


#endif /* SLATE_DETECT_HPP */
