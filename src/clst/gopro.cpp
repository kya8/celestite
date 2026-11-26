#include "detectors.hpp"
#include "gpmf.hpp"
#include "mp4/track.hpp"
#include "video_info.hpp"
#include <clst/log.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace clst {

namespace {

void read_setting(const gpmf::RecordView& record, CameraInfo& info)
{
    constexpr FourCC settings[]{
        "BITR"_fc, "DZOM"_fc, "DZST"_fc, "EISE"_fc, "EXMD"_fc, "EXMN"_fc, "EXMX"_fc, "EXPT"_fc,
        "FOCL"_fc, "HCTL"_fc, "HDRV"_fc, "PIMN"_fc, "PIMX"_fc, "PRJT"_fc, "PRTN"_fc, "PTCL"_fc,
        "PTEV"_fc, "PTSH"_fc, "PTWB"_fc, "RATE"_fc, "SROT"_fc, "ZFOV"_fc, "ZOOM"_fc,
    };
    if (!std::ranges::contains(settings, record.key)) {
        return;
    }
    if (const auto text = record.text()) {
        info.extras[GroupId::Metadata][fourcc_str(record.key)] = std::string(*text);
    } else if (record.repeat == 1 && record.numeric_elements_per_sample() == 1) {
        if (const auto number = record.number(0)) {
            info.extras[GroupId::Metadata][fourcc_str(record.key)] = *number;
        }
    }
}

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
    default: return read_setting(record, info);
    }

    const auto text = record.text();
    if (!text) {
        CLST_LOG_WARN("Invalid GPMF text in {}", fourcc_str(record.key));
        return;
    }
    auto& metadata = info.extras[GroupId::NormalizedMetadata];
    metadata[field] = std::string(*text);
    if (field == KeyId::StabilizationMode) {
        metadata[KeyId::HasStabilization] = *text != "N" && *text != "N/A";
    }
}

struct SensorStream {
    std::vector<double> values;
    std::vector<gpmf::SampleBatch> batches;
    bool failed = false;
};

std::size_t sensor_axes(FourCC key) noexcept
{
    switch (key) {
    case "CORI"_fc: case "IORI"_fc: return 4;
    case "GYRO"_fc: case "ACCL"_fc: case "GRAV"_fc: case "WRGB"_fc: return 3;
    case "SHUT"_fc: case "ISOE"_fc: case "ISOG"_fc: case "WBAL"_fc: case "SROT"_fc: return 1;
    default: return 0;
    }
}

template<std::size_t AxisCount, typename Convert>
auto timed_samples(const SensorStream& stream, std::span<const gpmf::SampleTiming> timing, Convert convert)
{
    using Sample = std::invoke_result_t<Convert, double, std::span<const double, AxisCount>>;
    std::vector<Sample> samples;
    samples.reserve(stream.values.size() / AxisCount);
    const auto* values = stream.values.data();
    for (std::size_t batch = 0; batch < stream.batches.size(); ++batch) {
        for (std::size_t index = 0; index < stream.batches[batch].count; ++index) {
            samples.push_back(convert(timing[batch].at(index), std::span<const double, AxisCount>(values, AxisCount)));
            values += AxisCount;
        }
    }
    return samples;
}

template<std::size_t AxisCount>
auto raw_sample(double timestamp, std::span<const double, AxisCount> values)
{
    return [&]<std::size_t... Indices>(std::index_sequence<Indices...>) {
        return std::tuple(timestamp, values[Indices]...);
    }(std::make_index_sequence<AxisCount>{});
}

types::Tuple4d camera_quaternion(std::span<const double, 4> values) noexcept
{
    // GoPro stores W,-X,-Y,-Z relative to the OpenCV camera frame.
    return {-values[1], -values[2], -values[3], values[0]};
}

types::Tuple5d timed_camera_quaternion(double timestamp, std::span<const double, 4> values) noexcept
{
    return std::tuple_cat(std::tuple(timestamp), camera_quaternion(values));
}

std::optional<std::vector<gpmf::SampleTiming>> stream_timing(
    FourCC key, const SensorStream& stream, std::optional<gpmf::SampleClock> clock)
{
    const auto missing = std::ranges::count_if(stream.batches, [](const auto& batch) { return !batch.timestamp; });
    if (missing != 0) {
        CLST_LOG_DEBUG("STMP is unavailable in {}/{} GoPro {} payloads. Estimating those timestamps.",
                       missing, stream.batches.size(), fourcc_str(key));
    }
    auto timing = gpmf::calculate_sample_timing(stream.batches, clock);
    if (!timing) {
        CLST_LOG_WARN("Could not estimate timestamps for GoPro {}", fourcc_str(key));
    }
    return timing;
}

void store_camera_orientation(VarMap& data, const SensorStream& stream, double frame_rate,
                              std::optional<gpmf::SampleClock> clock)
{
    const auto sample_count = stream.values.size() / 4;
    if (frame_rate > 0.0) {
        types::TimedQuaternionVec timed;
        timed.reserve(sample_count);
        // Some recordings carry extra CORI samples beyond the last video frame.
        for (std::size_t index = 0; index < sample_count; ++index) {
            timed.push_back(timed_camera_quaternion(index / frame_rate,
                std::span<const double, 4>(stream.values.data() + 4 * index, 4)));
        }
        data[KeyId::TimedCameraQuaternionData] = std::move(timed);
        return;
    }
    if (const auto timing = stream_timing("CORI"_fc, stream, clock)) {
        data[KeyId::TimedCameraQuaternionData] = timed_samples<4>(stream, *timing, timed_camera_quaternion);
        return;
    }

    // Preserve orientation samples when timing is unavailable.
    types::QuaternionVec quaternions;
    quaternions.reserve(sample_count);
    for (std::size_t index = 0; index < stream.values.size(); index += 4) {
        quaternions.push_back(camera_quaternion(std::span<const double, 4>(stream.values.data() + index, 4)));
    }
    data[KeyId::CameraQuaternionData] = std::move(quaternions);
}

void read_telemetry(mp4::Mp4Stream& file, CameraInfo& info, const VideoInfo* video_info)
{
    const auto tracks = mp4::get_tracks(file);
    const auto track = std::ranges::find(tracks, "gpmd"_fc, &mp4::TrackInfo::stsd_first_format);
    if (track == tracks.end()) {
        return;
    }

    std::map<FourCC, SensorStream> streams;
    mp4::SampleReader reader(file, *track);
    mp4::SampleReader::SampleInfo sample;
    const auto read_record = [&](const gpmf::RecordView& record, const gpmf::Modifiers& modifiers) {
        const auto axes = sensor_axes(record.key);
        if (axes == 0 || record.repeat == 0) {
            return;
        }
        auto& stream = streams[record.key];
        if (stream.failed) {
            return;
        }

        const auto batch = gpmf::sample_batch(record, modifiers, sample.timestamp, sample.duration);
        if (record.key == "CORI"_fc && stream.batches.empty()) {
            if (batch.timestamp) {
                CLST_LOG_DEBUG("First GoPro CORI sample has STMP {} (time origin)", *batch.timestamp);
            } else {
                CLST_LOG_DEBUG("STMP is unavailable for the first GoPro CORI sample");
            }
        }
        stream.batches.push_back(batch);

        // Preserve stored axis order for newly exposed streams.
        gpmf::Modifiers scale_only;
        scale_only.scale = modifiers.scale;
        const bool orient = record.key == "GYRO"_fc || record.key == "CORI"_fc;
        const auto values = gpmf::decode_scaled_samples(record, orient ? modifiers : scale_only, axes);
        if (!values) {
            CLST_LOG_WARN("Invalid or unsupported GPMF sensor data in {}", fourcc_str(record.key));
            stream.failed = true;
            stream.values.clear();
            return;
        }
        stream.values.insert(stream.values.end(), values->begin(), values->end());
    };
    while (reader.read_next(sample)) {
        if (const auto result = gpmf::visit_payload(sample.data, read_record); !result) {
            CLST_LOG_WARN("Malformed GPMF in timed sample {} ({})", sample.sample_idx, gpmf::describe(result.error()));
            return;
        }
    }

    const double frame_rate = video_info && video_info->is_cfr && std::isfinite(video_info->fps) && video_info->fps > 0.0
        ? video_info->fps : 0.0;
    std::optional<gpmf::SampleClock> clock;
    if (const auto cori = streams.find("CORI"_fc); cori != streams.end() && !cori->second.batches.empty()) {
        const double first_frame_packet = cori->second.batches.front().start;
        for (auto& [key, stream] : streams) {
            for (auto& batch : stream.batches) {
                batch.start -= first_frame_packet;
            }
        }
        clock = gpmf::estimate_sample_clock(cori->second.batches, cori->second.failed ? 0.0 : frame_rate);
    } else {
        CLST_LOG_DEBUG("STMP reference from GoPro CORI is unavailable");
    }
    // SHUT provides a first-frame reference on older cameras.
    for (const auto key : {"SHUT"_fc, "GYRO"_fc, "ACCL"_fc}) {
        if (clock) {
            break;
        }
        if (const auto stream = streams.find(key); stream != streams.end()) {
            clock = gpmf::estimate_sample_clock(stream->second.batches);
        }
    }

    for (const auto& [key, stream] : streams) {
        if (stream.failed || stream.values.empty()) {
            continue;
        }
        if (key == "CORI"_fc) {
            store_camera_orientation(info.extras[GroupId::SensorData], stream, frame_rate, clock);
            continue;
        }
        const auto timing = stream_timing(key, stream, clock);
        if (!timing) {
            continue;
        }
        auto& data = info.extras[GroupId::SensorData];
        switch (key) {
        case "GYRO"_fc:
            data[KeyId::GyroData] = timed_samples<3>(stream, *timing, [](double timestamp, auto values) {
                // Convert the GoPro ZXY axes to the OpenCV representation in rad/s.
                return types::Tuple4d(timestamp, -values[1], -values[0], -values[2]);
            });
            break;
        case "ACCL"_fc:
            data[KeyId::AccData] = timed_samples<3>(stream, *timing, raw_sample<3>);
            break;
        case "SHUT"_fc:
            data[KeyId::ExposureData] = timed_samples<1>(stream, *timing, raw_sample<1>);
            break;
        case "GRAV"_fc: case "WRGB"_fc:
            data[fourcc_str(key)] = timed_samples<3>(stream, *timing, raw_sample<3>);
            break;
        case "IORI"_fc:
            data[fourcc_str(key)] = timed_samples<4>(stream, *timing, raw_sample<4>);
            break;
        case "ISOE"_fc: case "ISOG"_fc: case "WBAL"_fc: case "SROT"_fc:
            data[fourcc_str(key)] = timed_samples<1>(stream, *timing, raw_sample<1>);
            break;
        }
    }
}

} // namespace

bool detect_gopro(mp4::Mp4Stream& file, CameraInfo& info, bool metadata_only, const VideoInfo* video_info) noexcept
{
    CLST_LOG_INFO("Probing GoPro...");
    types::VecBytes data;
    try {
        file.seek(ex, 0);
        const auto moov = file.seek_to_atom_data("moov"_fc, file.get_length(ex));
        const auto udta = file.seek_to_atom_data("udta"_fc, moov);
        const auto atom = file.seek_to_atom_data("GPMF"_fc, udta);
        if (atom.data_size() != 0) {
            data = file.read_atom_data(atom);
        }
    } catch (const StreamError&) {
        CLST_LOG_DEBUG("Could not read GoPro GPMF box");
        return false;
    } catch (const std::system_error&) {
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
    read_telemetry(file, info, video_info);
    return true;
}

} // namespace clst
