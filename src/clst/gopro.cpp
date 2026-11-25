#include "detectors.hpp"
#include "gpmf.hpp"
#include "mp4/track.hpp"
#include <clst/log.hpp>
#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace clst {

namespace {

void read_metadata(const gpmf::RecordView& record, CameraInfo& info)
{
    KeyId field;
    switch (record.key) {
    case "MINF"_fc: field = KeyId::CameraModel; break;
    case "CASN"_fc: field = KeyId::SerialNumber; break;
    case "VFOV"_fc: field = KeyId::LensType; break;
    case "FMWR"_fc: field = KeyId::FirmwareVersion; break;
    case "EISA"_fc: field = KeyId::StabilizationMode; break;
    case "OREN"_fc: field = KeyId::CameraRotation; break;
    default: return;
    }

    const auto text = record.text();
    if (!text) {
        CLST_LOG_WARN("Invalid GPMF text in {}", fourcc_str<false>(record.key));
        return;
    }
    auto& metadata = info.extras[GroupId::NormalizedMetadata];
    metadata[field] = std::string(*text);
    if (field == KeyId::StabilizationMode) {
        metadata[KeyId::HasStabilization] = *text != "N" && *text != "N/A";
    }
}

template<std::size_t AxisCount, typename Series, typename Convert>
void append_sensor_samples(const gpmf::RecordView& record, const gpmf::Modifiers& modifiers,
                           std::optional<Series>& series, Convert convert)
{
    if (!series) {
        return;
    }
    const auto values = gpmf::decode_scaled_samples(record, modifiers, AxisCount);
    if (!values) {
        CLST_LOG_WARN("Invalid or unsupported GPMF sensor data in {}", fourcc_str<false>(record.key));
        series.reset();
        return;
    }
    for (std::size_t index = 0; index < values->size(); index += AxisCount) {
        series->push_back(convert(std::span<const double, AxisCount>(values->data() + index, AxisCount)));
    }
}

void read_telemetry(mp4::Mp4Stream& file, CameraInfo& info)
{
    const auto tracks = mp4::get_tracks(file);
    const auto track = std::ranges::find(tracks, "gpmd"_fc, &mp4::TrackInfo::stsd_first_format);
    if (track == tracks.end()) {
        return;
    }

    std::optional<types::QuaternionVec> quaternions(std::in_place);
    std::optional<types::GyroVec> gyro_data(std::in_place);
    const auto read_record = [&](const gpmf::RecordView& record, const gpmf::Modifiers& modifiers) {
        switch (record.key) {
        case "CORI"_fc:
            append_sensor_samples<4>(record, modifiers, quaternions, [](auto sample) {
                // GoPro stores W,-X,-Y,-Z relative to the OpenCV camera frame.
                return types::Tuple4d(-sample[1], -sample[2], -sample[3], sample[0]);
            });
            break;
        case "GYRO"_fc:
            append_sensor_samples<3>(record, modifiers, gyro_data, [](auto sample) {
                // Convert the GoPro ZXY axes to the OpenCV representation in rad/s.
                return types::Tuple4d(0.0, -sample[1], -sample[0], -sample[2]);
            });
            break;
        }
    };

    mp4::SampleReader reader(file, *track);
    mp4::SampleReader::SampleInfo sample;
    while (reader.read_next(sample)) {
        if (const auto result = gpmf::visit_payload(sample.data, read_record); !result) {
            CLST_LOG_WARN("Malformed GPMF in timed sample {} ({})", sample.sample_idx, gpmf::describe(result.error()));
            return;
        }
    }

    if (quaternions && !quaternions->empty()) {
        info.extras[GroupId::SensorData][KeyId::CameraQuaternionData] = std::move(*quaternions);
    }
    if (gyro_data && !gyro_data->empty()) {
        info.extras[GroupId::SensorData][KeyId::GyroData] = std::move(*gyro_data);
    }
}

} // namespace

bool detect_gopro(mp4::Mp4Stream& file, CameraInfo& info, bool metadata_only) noexcept
{
    CLST_LOG_INFO("Probing GoPro...");
    types::VecBytes data;
    try {
        file.seek(0);
        const auto moov = file.seek_to_atom_data("moov"_fc, file.get_length());
        const auto udta = file.seek_to_atom_data("udta"_fc, moov);
        const auto atom = file.seek_to_atom_data("GPMF"_fc, udta);
        if (atom.data_size() != 0) {
            data = file.read_atom_data(atom);
        }
    } catch (const StreamError&) {
        CLST_LOG_DEBUG("Could not read GoPro GPMF box");
        return false;
    }

    info.vendor = CameraVendor::GoPro;
    const auto metadata_result = gpmf::visit_payload(data, [&](const gpmf::RecordView& record, const gpmf::Modifiers&) {
        read_metadata(record, info);
    });
    if (!metadata_result) {
        CLST_LOG_WARN("Malformed global GoPro GPMF data ({})", gpmf::describe(metadata_result.error()));
    }
    if (metadata_only) {
        return true;
    }

    info.extras[GroupId::Other]["GPMF"] = std::move(data);
    read_telemetry(file, info);
    return true;
}

} // namespace clst
