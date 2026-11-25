#ifndef CLST_DETECTORS_HPP
#define CLST_DETECTORS_HPP

#include <clst/camera_info.hpp>
#include "clst/mp4/mp4.hpp"

// All detectors should return true if the specified camera's signature is found. Otherwise, false is returned.

namespace clst {

bool
detect_gopro(mp4::Mp4Stream& file, CameraInfo& info, bool metadata_only = false) noexcept;

bool
detect_insta360(mp4::Mp4Stream& file, CameraInfo& info, bool metadata_only = false) noexcept;

} // namespace clst


#endif /* CLST_DETECTORS_HPP */
