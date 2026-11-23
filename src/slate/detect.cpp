#include <slate/detect.hpp>
#include <slate/log.hpp>
#include "detectors.hpp"
#include <sstream>
#include "slate/mp4/extract_video_info.hpp"

namespace slate {

using namespace mp4;

std::optional<CameraInfo>
detect(const char* video_file, bool metadata_only) noexcept
{
    Mp4Stream file;
    if (!file.open(video_file)) {
        SLATE_LOG_ERROR("Cannot open file: {}", video_file);
        return std::nullopt;
    }

    CameraInfo result{};
    const auto detect_success =
        detect_gopro   (file, result, metadata_only) ||
        detect_insta360(file, result, metadata_only);

    VideoInfo video_info{};
    types::VecI32 video_track_idx;
    const auto has_video_info = extract_video_info(file, video_info, &video_track_idx);

    if (!detect_success) {
        if (!has_video_info) {
            SLATE_LOG_ERROR("This is not a supported video file.");
            return std::nullopt;
        }
        SLATE_LOG_NOTICE("No known camera vendor was detected.");
    }
    if (has_video_info) {
        VarMap& map = result.extras[GroupId::VideoInfo];
        map[KeyId::Duration]        = video_info.duration;
        map[KeyId::Width]           = video_info.width;
        map[KeyId::Height]          = video_info.height;
        map[KeyId::DisplayRotation] = video_info.display_rotation;
        map[KeyId::FrameCount]      = video_info.nb_frames;
        map[KeyId::FPS]             = video_info.fps;
        map[KeyId::IsCFR]           = video_info.is_cfr;
        map[KeyId::VideoTrackIdx]   = std::move(video_track_idx);
    }

    return result;
}

std::string_view
to_string(CameraVendor type) noexcept
{
    switch(type)
    {
    case(CameraVendor::Unknown):   return "Unknown";
    case(CameraVendor::GoPro):     return "GoPro";
    case(CameraVendor::Insta360):  return "Insta360";
    }

    return {};
}

std::string
CameraInfo::describe() const noexcept
{
    std::ostringstream ret;
    ret << "Camera Vendor: " << to_string(vendor) << "; ";

    const auto metadata = extras.get(GroupId::NormalizedMetadata);
    if (metadata) {
        for (const auto& [key, val] : *metadata) {
            ret << to_string(key) << ": " << val << "; ";
        }
    }

    ret << "Extra info: " << extras.size();

    return std::move(ret).str();
}

} // namespace slate
