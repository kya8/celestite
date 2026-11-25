#ifndef CLST_MP4_EXTRACT_VIDEO_INFO_HPP
#define CLST_MP4_EXTRACT_VIDEO_INFO_HPP

#include "mp4.hpp"
#include "clst/video_info.hpp"
#include <clst/value_types.hpp>

namespace clst::mp4 {

bool extract_video_info(Mp4Stream& file, VideoInfo& out, types::VecI32* video_track_idx = nullptr) noexcept;

} /* namespace clst::mp4 */

#endif /* CLST_MP4_EXTRACT_VIDEO_INFO_HPP */
