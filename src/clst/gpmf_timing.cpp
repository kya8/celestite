#include "gpmf.hpp"
#include "streams/MemoryCursor.hpp"
#include <algorithm>
#include <cmath>

namespace clst::gpmf {

namespace {

constexpr double microsecond = 1e-6;

std::optional<std::uint64_t> read_timestamp(const std::optional<RecordView>& record) noexcept
{
    if (!record || (record->type != 'J' && record->type != 'j') || record->repeat != 1
        || record->sample_size != sizeof(std::uint64_t) || record->value.size() != sizeof(std::uint64_t)) {
        return std::nullopt;
    }
    Cursor<false> cursor(record->value.data(), sizeof(std::uint64_t));
    const auto value = cursor.read_num<Endian::BE, std::uint64_t>();
    if (!value || (record->type == 'j' && (*value >> 63) != 0)) {
        return std::nullopt;
    }
    return *value;
}

std::vector<std::uint64_t> sample_indices(std::span<const SampleBatch> batches)
{
    std::vector<std::uint64_t> indices;
    indices.reserve(batches.size());
    std::uint64_t next_index = 0;
    const SampleBatch* previous = nullptr;
    for (const auto& batch : batches) {
        // A counter can account for samples missing from an intervening payload.
        // Within contiguous payloads use the numbers actually present in the stream.
        if (previous && previous->total_samples && batch.total_samples
            && batch.start - (previous->start + previous->duration) > 1e-9) {
            const auto expected_total = std::uint64_t(*previous->total_samples) + batch.count;
            if (*batch.total_samples > expected_total) {
                next_index += *batch.total_samples - expected_total;
            }
        }
        indices.push_back(next_index);
        next_index += batch.count;
        previous = &batch;
    }
    return indices;
}

std::optional<SampleTiming> fit_packet_timing(std::span<const SampleBatch> batches,
                                              std::span<const std::uint64_t> indices)
{
    for (const auto& batch : batches) {
        if (batch.count == 0 || !std::isfinite(batch.start + batch.duration) || batch.duration < 0.0) {
            return std::nullopt;
        }
    }
    const double first = batches.front().start;
    const double elapsed = batches.back().start + batches.back().duration - first;
    const double sample_count = double(indices.back() + batches.back().count);
    if (elapsed == 0.0 && sample_count == 1.0) {
        return SampleTiming{first, 0.0};
    }
    if (elapsed <= 0.0 || !std::isfinite(elapsed)) {
        return std::nullopt;
    }

    const SampleTiming average{first, elapsed / sample_count};
    if (batches.size() == 1) {
        return average;
    }

    // Fit the cumulative count at each packet's end to avoid inheriting packetization jitter.
    double mean_time = 0.0;
    double mean_count = 0.0;
    for (std::size_t index = 0; index < batches.size(); ++index) {
        const auto& batch = batches[index];
        mean_time += batch.start + batch.duration - first;
        mean_count += double(indices[index] + batch.count);
    }
    mean_time /= batches.size();
    mean_count /= batches.size();

    double covariance = 0.0;
    double variance = 0.0;
    for (std::size_t index = 0; index < batches.size(); ++index) {
        const auto& batch = batches[index];
        const double time_delta = batch.start + batch.duration - first - mean_time;
        covariance += time_delta * (double(indices[index] + batch.count) - mean_count);
        variance += time_delta * time_delta;
    }
    if (covariance <= 0.0 || variance <= 0.0) {
        return average;
    }
    const double period = variance / covariance;
    return SampleTiming{first + mean_time - mean_count * period, period};
}

struct Anchor {
    std::uint64_t index;
    double time;
};

double tick_difference(std::uint64_t timestamp, std::uint64_t reference) noexcept
{
    return timestamp < reference ? -double(reference - timestamp) : double(timestamp - reference);
}

} // namespace

SampleBatch sample_batch(const RecordView& record, const Modifiers& modifiers, double start, double duration) noexcept
{
    SampleBatch batch{start, duration, record.repeat, read_timestamp(modifiers.timestamp)};
    if (const auto& total = modifiers.total_samples; total && total->type == 'L' && total->repeat == 1
        && total->numeric_elements_per_sample() == 1) {
        if (const auto value = total->number(0)) {
            batch.total_samples = static_cast<std::uint32_t>(*value);
        }
    }
    if (const auto& offset = modifiers.time_offset; offset && offset->repeat == 1
        && offset->numeric_elements_per_sample() == 1) {
        if (const auto value = offset->number(0); value && std::isfinite(*value)) {
            batch.delay = *value;
        }
    }
    return batch;
}

std::optional<SampleClock> estimate_sample_clock(std::span<const SampleBatch> batches, double frame_rate)
{
    const auto first = std::ranges::find_if(batches, [](const SampleBatch& batch) { return batch.timestamp.has_value(); });
    if (first == batches.end()) {
        return std::nullopt;
    }

    const bool frame_aligned = std::isfinite(frame_rate) && frame_rate > 0.0;
    const auto first_index = static_cast<std::size_t>(first - batches.begin());
    const auto indices = frame_aligned ? sample_indices(batches) : std::vector<std::uint64_t>{};
    const double first_time = frame_aligned ? double(indices[first_index]) / frame_rate : first->start;
    SampleClock clock{*first->timestamp, first_time, microsecond};
    for (auto last = batches.end(); last != first + 1;) {
        --last;
        if (!last->timestamp || *last->timestamp <= clock.timestamp || last->start <= first->start) {
            continue;
        }
        const double ticks = double(*last->timestamp - clock.timestamp);
        const double packet_elapsed = last->start - first->start;
        // STMP normally uses microseconds. Compare the elapsed clock to MP4 time to
        // recognize other decimal units, as in GoPro's sample-rate utility.
        for (double unit = 1e-9; unit < 1.1; unit *= 10.0) {
            const double elapsed = ticks * unit;
            if (std::abs(elapsed - packet_elapsed) > 0.1 * packet_elapsed) {
                continue;
            }
            clock.seconds_per_tick = unit;
            if (frame_aligned) {
                const auto last_index = static_cast<std::size_t>(last - batches.begin());
                const double frame_elapsed = double(indices[last_index] - indices[first_index]) / frame_rate;
                if (std::abs(elapsed - frame_elapsed) <= 0.1 * elapsed) {
                    clock.seconds_per_tick = frame_elapsed / ticks;
                }
            }
            return clock;
        }
    }
    return clock;
}

std::optional<std::vector<SampleTiming>> calculate_sample_timing(
    std::span<const SampleBatch> batches, std::optional<SampleClock> clock)
{
    std::vector<SampleTiming> result;
    if (batches.empty()) {
        return result;
    }
    const auto indices = sample_indices(batches);
    const auto fallback = fit_packet_timing(batches, indices);
    if (!fallback) {
        return std::nullopt;
    }
    if (!clock) {
        clock = estimate_sample_clock(batches);
    }

    std::vector<Anchor> anchors;
    if (clock) {
        for (std::size_t index = 0; index < batches.size(); ++index) {
            const auto& batch = batches[index];
            if (!batch.timestamp) {
                continue;
            }
            const auto time = clock->time + tick_difference(*batch.timestamp, clock->timestamp) * clock->seconds_per_tick;
            // Reject resets and timestamps inconsistent with the enclosing packet's clock.
            if (std::abs(time - batch.start) <= std::max(1.0, batch.duration)
                && (anchors.empty() || time > anchors.back().time)) {
                anchors.push_back({indices[index], time});
            }
        }
    }

    result.reserve(batches.size());
    std::size_t segment = 0;
    for (std::size_t index = 0; index < batches.size(); ++index) {
        const auto first_sample = indices[index];
        double time = fallback->at(first_sample);
        double period = fallback->period;
        if (anchors.size() == 1) {
            time = anchors.front().time + tick_difference(first_sample, anchors.front().index) * period;
        } else if (anchors.size() > 1) {
            while (segment + 2 < anchors.size() && first_sample >= anchors[segment + 1].index) {
                ++segment;
            }
            const auto& begin = anchors[segment];
            const auto& end = anchors[segment + 1];
            period = (end.time - begin.time) / double(end.index - begin.index);
            time = begin.time + tick_difference(first_sample, begin.index) * period;
        }
        result.push_back({time - batches[index].delay, period});
    }
    return result;
}

} // namespace clst::gpmf
