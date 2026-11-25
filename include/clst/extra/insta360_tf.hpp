#ifndef CLST_EXTRA_INSTA360_TF_HPP
#define CLST_EXTRA_INSTA360_TF_HPP

#include <Eigen/Core>
#include <clst/fwd.hpp>
#include <optional>
#include "clst/export.h"

namespace clst::insta360 {

CLST_EXPORT [[nodiscard]] const Eigen::Matrix3d* get_gyro_to_body_transform(const CameraInfo& info) noexcept;

// Coord transform from body frame to lens0 and lens1 (rotational part)
CLST_EXPORT [[nodiscard]] std::optional<Eigen::Matrix3d> get_R0(const double* offset_v3, int len) noexcept;
CLST_EXPORT [[nodiscard]] std::optional<Eigen::Matrix3d> get_R1(const double* offset_v3, int len) noexcept;
// Coord transform from lens0 to lens1 (translational part)
CLST_EXPORT [[nodiscard]] std::optional<Eigen::Vector3d> get_T01(const double* offset_v3, int len) noexcept;

} // namespace clst::insta360


#endif /* CLST_EXTRA_INSTA360_TF_HPP */
