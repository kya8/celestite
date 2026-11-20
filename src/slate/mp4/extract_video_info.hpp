#ifndef SLATE_MP4_EXTRACT_VIDEO_INFO_HPP
#define SLATE_MP4_EXTRACT_VIDEO_INFO_HPP

#include "mp4.hpp"
#include "slate/video_info.hpp"
#include <slate/value_types.hpp>

namespace slate::mp4 {

bool extract_video_info(Mp4Stream& file, VideoInfo& out, types::VecI32* video_track_ids = nullptr) noexcept;

} /* namespace slate::mp4 */

#endif /* SLATE_MP4_EXTRACT_VIDEO_INFO_HPP */
