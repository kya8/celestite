#ifndef CLST_ID_HPP
#define CLST_ID_HPP

#include "clst/export.h"
#include <string_view>

namespace clst {

// Well-known group id.
enum class GroupId {
    NormalizedMetadata, // Holds well-known metadata represented by KeyId.
    Metadata,
    SensorData,
    ProcessedData,
    VideoInfo,
    Other,
    Max_GroupId // sentinel
};

// Well-know key tag for commonly used types of metadata.
enum class KeyId {
    CameraModel,
    SubModel,
    SerialNumber,
    LensType,
    FirmwareVersion,
    StabilizationMode,
    HasStabilization,
    CameraRotation,
    LensParams,

    Width,
    Height,
    FPS,
    Duration,
    IsCFR,
    FrameCount,
    DisplayRotation,
    VideoTrackIdx,

    GyroData,
    AccData,
    ExposureData,
    CameraQuaternionData,
    TimedCameraQuaternionData,
    TimelapseTimestamp,
    GpsData,
    Max_KeyId // sentinel
};

/**
 * Get a informative string of the given GroupId.
 */
CLST_EXPORT [[nodiscard]] std::string_view to_string(GroupId id) noexcept;
/**
 * Get a informative string of the given KeyId.
 */
CLST_EXPORT [[nodiscard]] std::string_view to_string(KeyId id) noexcept;

} // namespace clst

#endif /* CLST_ID_HPP */
