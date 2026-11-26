#include "test.h"
#include "clst/gpmf.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace gpmf = clst::gpmf;
using clst::operator""_fc;

using Bytes = std::vector<unsigned char>;

void append_record(Bytes& bytes, std::string_view key, unsigned char type, std::uint8_t sample_size,
                   std::span<const unsigned char> value)
{
    CLST_ASSERT(key.size() == 4);
    CLST_ASSERT(sample_size > 0 && value.size() % sample_size == 0);
    const auto repeat = value.size() / sample_size;
    CLST_ASSERT(repeat <= 0xffff);
    bytes.insert(bytes.end(), key.begin(), key.end());
    bytes.push_back(type);
    bytes.push_back(sample_size);
    bytes.push_back(static_cast<unsigned char>(repeat >> 8));
    bytes.push_back(static_cast<unsigned char>(repeat & 0xff));
    bytes.insert(bytes.end(), value.begin(), value.end());
    bytes.insert(bytes.end(), (4 - value.size() % 4) % 4, 0xa5);
}

void append_record(Bytes& bytes, std::string_view key, unsigned char type, std::uint8_t sample_size,
                   std::initializer_list<unsigned char> value)
{
    append_record(bytes, key, type, sample_size, std::span(value.begin(), value.size()));
}

void append_container(Bytes& bytes, std::string_view key, const Bytes& value)
{
    append_record(bytes, key, 0, 4, value);
}

gpmf::RecordView read_record(gpmf::Reader& reader)
{
    const auto result = reader.next();
    CLST_ASSERT(result && *result);
    return **result;
}

void check_end(gpmf::Reader& reader)
{
    const auto result = reader.next();
    CLST_ASSERT(result && !*result);
}

void check_nested_and_padded_records()
{
    Bytes stream;
    append_record(stream, "MINF", 'c', 1, {'A', 0, 'B'});
    Bytes device;
    append_container(device, "STRM", stream);
    Bytes payload{0xa5};
    append_container(payload, "DEVC", device);
    append_container(payload, "DEVC", {});
    payload.insert(payload.end(), 5, 0);

    gpmf::Reader reader(std::span(payload).subspan(1));
    const auto root = read_record(reader);
    CLST_ASSERT(root.key == "DEVC"_fc && root.is_container());
    CLST_ASSERT(root.sample_size == 4 && root.repeat == device.size() / 4);
    auto children = reader.children(root);
    CLST_ASSERT(children);
    const auto nested = read_record(*children);
    CLST_ASSERT(nested.key == "STRM"_fc);
    auto records = children->children(nested);
    CLST_ASSERT(records);

    const auto model = read_record(*records);
    CLST_ASSERT(model.key == "MINF"_fc && model.type == 'c');
    CLST_ASSERT(model.value.size() == 3 && model.value.back() == 'B');
    CLST_ASSERT(model.text() == "A");
    check_end(*records);
    check_end(*children);
    CLST_ASSERT(read_record(reader).key == "DEVC"_fc);
    check_end(reader);
}

void check_malformed_records()
{
    const Bytes short_header{'M', 'I', 'N', 'F', 'c', 1, 0};
    const auto truncated_header = gpmf::Reader(short_header).next();
    CLST_ASSERT(!truncated_header && truncated_header.error() == gpmf::ParseError::truncated_header);

    const Bytes missing_padding{'M', 'I', 'N', 'F', 'c', 1, 0, 1, 'A'};
    const auto truncated_value = gpmf::Reader(missing_padding).next();
    CLST_ASSERT(!truncated_value && truncated_value.error() == gpmf::ParseError::truncated_value);

    Bytes payload;
    const Bytes child_header{'M', 'I', 'N', 'F', 'c', 1, 0, 1};
    append_container(payload, "DEVC", child_header);
    append_container(payload, "DEVC", {});
    gpmf::Reader reader(payload);
    const auto first = read_record(reader);
    auto children = reader.children(first);
    CLST_ASSERT(children);
    const auto child = children->next();
    CLST_ASSERT(!child && child.error() == gpmf::ParseError::truncated_value);
    CLST_ASSERT(read_record(reader).key == "DEVC"_fc);
    check_end(reader);

    payload.clear();
    append_record(payload, "DEVC", 0, 1, {1, 0, 0});
    const auto ignore_record = [](const gpmf::RecordView&, const gpmf::Modifiers&) {};
    const auto invalid_container = gpmf::visit_payload(payload, ignore_record);
    CLST_ASSERT(!invalid_container && invalid_container.error() == gpmf::ParseError::invalid_container);

    Bytes nested;
    for (int depth = 1; depth <= 16; ++depth) {
        Bytes container;
        append_container(container, depth == 16 ? "DEVC" : "STRM", nested);
        nested = std::move(container);
    }
    CLST_ASSERT(gpmf::visit_payload(nested, ignore_record));
    Bytes too_deep;
    append_container(too_deep, "DEVC", nested);
    const auto limit = gpmf::visit_payload(too_deep, ignore_record);
    CLST_ASSERT(!limit && limit.error() == gpmf::ParseError::nesting_limit);
}

void check_values()
{
    Bytes payload;
    append_record(payload, "GYRO", 's', 6, {0xff, 0x80, 0, 10, 0, 60, 0, 1, 0, 2, 0, 3});
    append_record(payload, "LONG", 'J', 8, {0, 0, 0, 1, 0, 0, 0, 0});
    append_record(payload, "FLOT", 'f', 4, {0xbf, 0xc0, 0, 0});
    append_record(payload, "DBLE", 'd', 8, {0x3f, 0xf8, 0, 0, 0, 0, 0, 0});
    append_record(payload, "FIXD", 'q', 4, {0xff, 0xfe, 0x80, 0});
    append_record(payload, "MANY", 'B', 1, Bytes(257, 0xac));
    for (const unsigned char type : {'?', '#', '~'}) {
        append_record(payload, "cust", type, 1, {0x12});
    }

    gpmf::Reader reader(payload);
    const auto gyro = read_record(reader);
    CLST_ASSERT(gyro.numeric_elements_per_sample() == 3 && gyro.repeat == 2);
    CLST_ASSERT(gyro.number(0) == -128.0 && gyro.number(5) == 3.0);
    CLST_ASSERT(!gyro.number(6) && !gyro.text());
    CLST_ASSERT(read_record(reader).number(0) == 4294967296.0);
    CLST_ASSERT(read_record(reader).number(0) == -1.5);
    CLST_ASSERT(read_record(reader).number(0) == 1.5);
    CLST_ASSERT(read_record(reader).number(0) == -1.5);
    const auto many = read_record(reader);
    CLST_ASSERT(many.repeat == 257 && many.number(256) == 0xac && !many.number(257));
    for (const unsigned char type : {'?', '#', '~'}) {
        const auto opaque = read_record(reader);
        CLST_ASSERT(opaque.key == "cust"_fc && opaque.type == type);
        CLST_ASSERT(opaque.value.size() == 1 && opaque.value.front() == 0x12);
        CLST_ASSERT(!opaque.text() && !opaque.number(0));
    }
    check_end(reader);
}

void check_modifiers()
{
    const Bytes readings{0, 20, 0, 60, 0, 120};
    Bytes device;
    append_record(device, "SCAL", 's', 2, {0, 9});

    Bytes first_stream;
    append_record(first_stream, "SCAL", 's', 2, {0, 2});
    append_record(first_stream, "SCAL", 's', 2, {0, 4});
    append_record(first_stream, "GYRO", 's', 6, readings);
    append_container(device, "STRM", first_stream);

    Bytes second_stream;
    append_record(second_stream, "GYRO", 's', 6, readings);
    append_record(second_stream, "SCAL", 's', 6, {0, 10, 0, 20, 0, 30});
    append_record(second_stream, "MTRX", 's', 2, {0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1});
    append_record(second_stream, "ORIN", 'c', 3, {'z', 'X', 'y'});
    append_record(second_stream, "ORIO", 'c', 1, {'X', 'Y', 'Z'});
    append_record(second_stream, "GYRO", 's', 6, readings);
    append_container(device, "STRM", second_stream);

    Bytes third_stream;
    append_record(third_stream, "MTRX", 's', 6, {0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1});
    append_record(third_stream, "ORIN", 'c', 1, {'z', 'X', 'y'});
    append_record(third_stream, "ORIO", 'c', 1, {'X', 'Y', 'Z'});
    append_record(third_stream, "GYRO", 's', 6, readings);
    append_container(device, "STRM", third_stream);

    Bytes fourth_stream;
    append_record(fourth_stream, "SCAL", 's', 2, {0, 0});
    append_record(fourth_stream, "GYRO", 's', 6, readings);
    append_container(device, "STRM", fourth_stream);

    Bytes payload;
    append_container(payload, "DEVC", device);
    std::vector<std::optional<std::vector<double>>> readings_by_record;
    const auto result = gpmf::visit_payload(payload, [&](const gpmf::RecordView& record, const gpmf::Modifiers& modifiers) {
        if (record.key == "GYRO"_fc) {
            readings_by_record.push_back(gpmf::decode_scaled_samples(record, modifiers, 3));
            CLST_ASSERT(!gpmf::decode_scaled_samples(record, modifiers, 4));
        }
    });
    CLST_ASSERT(result && readings_by_record.size() == 5);
    CLST_ASSERT((readings_by_record[0] == std::vector{10.0, 30.0, 60.0}));
    CLST_ASSERT((readings_by_record[1] == std::vector{20.0, 60.0, 120.0}));
    CLST_ASSERT((readings_by_record[2] == std::vector{3.0, -4.0, -2.0}));
    CLST_ASSERT((readings_by_record[3] == std::vector{40.0, 60.0, 120.0}));
    CLST_ASSERT(!readings_by_record[4]);
}

void check_timing_modifiers()
{
    Bytes timed_stream;
    append_record(timed_stream, "STMP", 'J', 8, {0, 0x20, 0, 0, 0, 0, 0, 1});
    append_record(timed_stream, "TSMP", 'L', 4, {0, 0, 1, 0x90});
    append_record(timed_stream, "TIMO", 'f', 4, {0x3e, 0, 0, 0});
    append_record(timed_stream, "GYRO", 's', 6, {0, 20, 0, 60, 0, 120});
    Bytes untimed_stream;
    append_record(untimed_stream, "ACCL", 's', 6, {0, 20, 0, 60, 0, 120});
    Bytes device;
    append_container(device, "STRM", timed_stream);
    append_container(device, "STRM", untimed_stream);
    Bytes payload;
    append_container(payload, "DEVC", device);

    std::size_t checked = 0;
    const auto result = gpmf::visit_payload(payload, [&](const gpmf::RecordView& record, const gpmf::Modifiers& modifiers) {
        if (record.key == "GYRO"_fc) {
            const auto batch = gpmf::sample_batch(record, modifiers, 2.0, 1.0);
            CLST_ASSERT(batch.timestamp == 9007199254740993ULL);
            CLST_ASSERT(batch.total_samples == 400 && batch.delay == 0.125);
            CLST_ASSERT(batch.start == 2.0 && batch.duration == 1.0 && batch.count == 1);
            ++checked;
        } else if (record.key == "ACCL"_fc) {
            const auto batch = gpmf::sample_batch(record, modifiers, 2.0, 1.0);
            CLST_ASSERT(!batch.timestamp && !batch.total_samples && batch.delay == 0.0);
            ++checked;
        }
    });
    CLST_ASSERT(result && checked == 2);
}

void check_close(double actual, double expected)
{
    CLST_ASSERT(std::abs(actual - expected) < 1e-10);
}

void check_timestamped_timing()
{
    constexpr std::uint64_t reference = 9007199254740993ULL;
    const std::array cori{
        gpmf::SampleBatch{0.0, 1.001, 24, reference},
        gpmf::SampleBatch{1.001, 1.001, 24, reference + 1000000},
    };
    const auto clock = gpmf::estimate_sample_clock(cori, 24000.0 / 1001.0);
    CLST_ASSERT(clock && clock->timestamp == reference && clock->time == 0.0);
    check_close(clock->seconds_per_tick, 1.001e-6);

    const std::array gyro{
        gpmf::SampleBatch{0.0, 1.001, 2, reference - 25000},
        gpmf::SampleBatch{1.001, 1.001, 2, {}, {}, 0.125},
        gpmf::SampleBatch{2.002, 1.001, 2, reference + 1975000},
        gpmf::SampleBatch{3.003, 1.001, 2, 0}, // reset timestamp
    };
    const auto timing = gpmf::calculate_sample_timing(gyro, clock);
    CLST_ASSERT(timing && timing->size() == gyro.size());
    check_close((*timing)[0].at(0), -0.025025);
    check_close((*timing)[0].at(1), 0.475475);
    check_close((*timing)[1].at(0), 0.975975 - 0.125);
    check_close((*timing)[2].at(0), 1.976975);
    check_close((*timing)[3].at(0), 2.977975);

    const std::array nanoseconds{
        gpmf::SampleBatch{0.0, 1.0, 100, reference},
        gpmf::SampleBatch{1.0, 1.0, 100, reference + 1000000000},
    };
    const auto nanosecond_clock = gpmf::estimate_sample_clock(nanoseconds);
    CLST_ASSERT(nanosecond_clock);
    check_close(nanosecond_clock->seconds_per_tick, 1e-9);
    const auto nanosecond_timing = gpmf::calculate_sample_timing(nanoseconds);
    CLST_ASSERT(nanosecond_timing);
    check_close((*nanosecond_timing)[0].period, 0.01);

    const std::array missing_first_stamp{
        gpmf::SampleBatch{0.0, 1.001, 24},
        gpmf::SampleBatch{1.001, 1.001, 24, reference + 1000000},
        gpmf::SampleBatch{2.002, 1.001, 24, reference + 2000000},
    };
    const auto recovered = gpmf::estimate_sample_clock(missing_first_stamp, 24000.0 / 1001.0);
    CLST_ASSERT(recovered && recovered->timestamp == reference + 1000000);
    check_close(recovered->time, 1.001);
    const auto recovered_timing = gpmf::calculate_sample_timing(missing_first_stamp, recovered);
    CLST_ASSERT(recovered_timing);
    check_close((*recovered_timing)[0].start, 0.0);
}

void check_packet_timing()
{
    const std::array uneven{
        gpmf::SampleBatch{0.0, 1.0, 3},
        gpmf::SampleBatch{1.0, 1.0, 5},
        gpmf::SampleBatch{2.0, 1.0, 4},
    };
    const auto fit = gpmf::calculate_sample_timing(uneven);
    CLST_ASSERT(fit && fit->size() == uneven.size());
    check_close((*fit)[0].period, 1.0 / 4.5);
    check_close((*fit)[1].start, (*fit)[0].at(3));
    check_close((*fit)[2].start, (*fit)[1].at(5));

    const std::array missing{
        gpmf::SampleBatch{0.0, 1.0, 4, {}, 104},
        gpmf::SampleBatch{2.0, 1.0, 4, {}, 112},
    };
    const auto gap = gpmf::calculate_sample_timing(missing);
    CLST_ASSERT(gap && gap->size() == missing.size());
    check_close((*gap)[0].start, 0.0);
    check_close((*gap)[0].period, 0.25);
    check_close((*gap)[1].start, 2.0);

    const std::array single{gpmf::SampleBatch{1.5, 0.5, 100}};
    const auto short_clip = gpmf::calculate_sample_timing(single);
    CLST_ASSERT(short_clip);
    check_close(short_clip->front().start, 1.5);
    check_close(short_clip->front().period, 0.005);
}

} // namespace

int gpmf(int, char*[])
{
    check_nested_and_padded_records();
    check_malformed_records();
    check_values();
    check_modifiers();
    check_timing_modifiers();
    check_timestamped_timing();
    check_packet_timing();
    return 0;
}
