#include "gpmf.hpp"
#include "streams/MemoryCursor.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <numeric>
#include <utility>

namespace clst::gpmf {

namespace {

constexpr std::size_t alignment = 4;
constexpr std::size_t header_size = 8;
constexpr std::size_t max_depth = 16;

constexpr std::size_t numeric_type_size(unsigned char type) noexcept
{
    switch (type) {
    case 'b': case 'B': return 1;
    case 's': case 'S': return 2;
    case 'l': case 'L': case 'f': case 'q': return 4;
    case 'j': case 'J': case 'd': case 'Q': return 8;
    default: return 0;
    }
}

std::optional<std::vector<double>> read_finite_numbers(const RecordView& record)
{
    const auto count = record.numeric_elements_per_sample() * record.repeat;
    std::vector<double> values;
    values.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const auto value = record.number(index);
        if (!value || !std::isfinite(*value)) {
            return std::nullopt;
        }
        values.push_back(*value);
    }
    return values;
}

// Returns an index for an ASCII axis letter, with 26 marking an invalid label.
constexpr std::size_t axis_index(char axis) noexcept
{
    if (axis >= 'A' && axis <= 'Z') {
        return axis - 'A';
    }
    if (axis >= 'a' && axis <= 'z') {
        return axis - 'a';
    }
    return 26;
}

std::vector<double> orientation_matrix(const Modifiers& modifiers, std::size_t axis_count)
{
    const auto from = modifiers.orientation_in ? modifiers.orientation_in->text() : std::nullopt;
    const auto to = modifiers.orientation_out ? modifiers.orientation_out->text() : std::nullopt;
    if (!from || !to || from->size() != axis_count || to->size() != axis_count) {
        return {};
    }

    // Letter identifies the axis and case identifies its sign.
    std::array<std::size_t, 26> columns;
    columns.fill(axis_count);
    for (std::size_t column = 0; column < axis_count; ++column) {
        const auto index = axis_index((*from)[column]);
        if (index >= columns.size() || columns[index] != axis_count) {
            return {};
        }
        columns[index] = column;
    }

    std::vector<double> matrix(axis_count * axis_count);
    for (std::size_t row = 0; row < axis_count; ++row) {
        const auto index = axis_index((*to)[row]);
        if (index >= columns.size() || columns[index] == axis_count) {
            return {};
        }
        const auto column = columns[index];
        matrix[row * axis_count + column] = (*to)[row] == (*from)[column] ? 1.0 : -1.0;
        columns[index] = axis_count;
    }
    return matrix;
}

} // namespace

std::string_view describe(ParseError error) noexcept
{
    switch (error) {
    case ParseError::truncated_header: return "truncated KLV header";
    case ParseError::invalid_key: return "invalid FourCC";
    case ParseError::invalid_sample_size: return "invalid sample size";
    case ParseError::truncated_value: return "truncated KLV value or padding";
    case ParseError::invalid_container: return "invalid KLV container";
    case ParseError::nesting_limit: return "nesting limit exceeded";
    case ParseError::expected_device: return "expected a DEVC container";
    }
    return "invalid GPMF payload";
}

std::optional<std::string_view> RecordView::text() const noexcept
{
    if (type != 'c' && type != 'u' && (type != 'F' || sample_size % 4 != 0)) {
        return std::nullopt;
    }
    if (value.empty()) {
        return std::string_view{};
    }
    const std::string_view bytes(reinterpret_cast<const char*>(value.data()), value.size());
    return bytes.substr(0, bytes.find('\0'));
}

std::size_t RecordView::numeric_elements_per_sample() const noexcept
{
    const auto width = numeric_type_size(type);
    return width != 0 && sample_size % width == 0 ? sample_size / width : 0;
}

std::optional<double> RecordView::number(std::size_t index) const noexcept
{
    const auto width = numeric_type_size(type);
    if (width == 0 || index >= numeric_elements_per_sample() * repeat || index >= value.size() / width) {
        return std::nullopt;
    }

    Cursor<false> cursor(value.data() + index * width, static_cast<std::ptrdiff_t>(width));
    switch (type) {
    case 'b': return cursor.read_num<Endian::BE, std::int8_t>();
    case 'B': return cursor.read_num<Endian::BE, std::uint8_t>();
    case 's': return cursor.read_num<Endian::BE, std::int16_t>();
    case 'S': return cursor.read_num<Endian::BE, std::uint16_t>();
    case 'l': return cursor.read_num<Endian::BE, std::int32_t>();
    case 'L': return cursor.read_num<Endian::BE, std::uint32_t>();
    case 'j': return static_cast<double>(cursor.read_num<Endian::BE, std::int64_t>());
    case 'J': return static_cast<double>(cursor.read_num<Endian::BE, std::uint64_t>());
    case 'f': return std::bit_cast<float>(cursor.read_num<Endian::BE, std::uint32_t>());
    case 'd': return std::bit_cast<double>(cursor.read_num<Endian::BE, std::uint64_t>());
    case 'q': return cursor.read_num<Endian::BE, std::int32_t>() / 0x1p16;
    case 'Q': return static_cast<double>(cursor.read_num<Endian::BE, std::int64_t>()) / 0x1p32;
    default: return std::nullopt;
    }
}

std::expected<std::optional<RecordView>, ParseError> Reader::next() noexcept
{
    const auto is_zero = [](unsigned char byte) { return byte == 0; };
    while (remaining_.size() >= alignment && std::ranges::all_of(remaining_.first(alignment), is_zero)) {
        remaining_ = remaining_.subspan(alignment);
    }
    if (remaining_.size() < alignment && std::ranges::all_of(remaining_, is_zero)) {
        remaining_ = {};
        return std::optional<RecordView>{};
    }
    if (remaining_.size() < header_size) {
        return std::unexpected(ParseError::truncated_header);
    }
    if (!std::ranges::all_of(remaining_.first(alignment), [](unsigned char byte) {
        return byte >= 0x20 && byte <= 0x7e;
    })) {
        return std::unexpected(ParseError::invalid_key);
    }

    Cursor<false> cursor(remaining_.data(), header_size);
    const auto [key, type, sample_size, repeat] =
        cursor.read_nums<Endian::BE, FourCC, unsigned char, std::uint8_t, std::uint16_t>();
    if (sample_size == 0 && repeat != 0) {
        return std::unexpected(ParseError::invalid_sample_size);
    }

    // An eight-bit size and a sixteen-bit repeat leave room for rounding in size_t.
    const auto value_size = std::size_t(sample_size) * repeat;
    const auto padded_size = (value_size + alignment - 1) & ~(alignment - 1);
    if (padded_size > remaining_.size() - header_size) {
        return std::unexpected(ParseError::truncated_value);
    }

    const RecordView record{key, type, sample_size, repeat, remaining_.subspan(header_size, value_size)};
    remaining_ = remaining_.subspan(header_size + padded_size);
    return record;
}

std::expected<Reader, ParseError> Reader::children(const RecordView& record) const noexcept
{
    if (!record.is_container() || record.value.size() % alignment != 0) {
        return std::unexpected(ParseError::invalid_container);
    }
    if (depth_ == max_depth) {
        return std::unexpected(ParseError::nesting_limit);
    }

    Reader reader(record.value);
    reader.depth_ = depth_ + 1;
    return reader;
}

void Modifiers::observe(const RecordView& record) noexcept
{
    std::optional<RecordView>* modifier;
    switch (record.key) {
    case "SCAL"_fc: modifier = &scale; break;
    case "MTRX"_fc: modifier = &matrix; break;
    case "ORIN"_fc: modifier = &orientation_in; break;
    case "ORIO"_fc: modifier = &orientation_out; break;
    case "STMP"_fc: modifier = &timestamp; break;
    case "TSMP"_fc: modifier = &total_samples; break;
    case "TIMO"_fc: modifier = &time_offset; break;
    default: return;
    }
    if (!*modifier) {
        *modifier = record;
    }
}

std::optional<std::vector<double>> decode_scaled_samples(
    const RecordView& record, const Modifiers& modifiers, std::size_t axis_count)
{
    if (axis_count == 0 || record.numeric_elements_per_sample() != axis_count) {
        return std::nullopt;
    }

    std::vector<double> divisors{1.0};
    if (modifiers.scale) {
        const auto& scale = *modifiers.scale;
        const auto count = scale.numeric_elements_per_sample() * scale.repeat;
        if (count != 1 && count != axis_count) {
            return std::nullopt;
        }
        auto values = read_finite_numbers(scale);
        if (!values || std::ranges::find(*values, 0.0) != values->end()) {
            return std::nullopt;
        }
        divisors = std::move(*values);
    }

    std::vector<double> matrix;
    if (modifiers.matrix) {
        const auto& calibration = *modifiers.matrix;
        if (calibration.numeric_elements_per_sample() * calibration.repeat != axis_count * axis_count) {
            return std::nullopt;
        }
        auto values = read_finite_numbers(calibration);
        if (!values) {
            return std::nullopt;
        }
        if (std::ranges::any_of(*values, [](double value) { return value != -1.0 && value != 0.0 && value != 1.0; })) {
            matrix = std::move(*values);
        }
    }
    if (matrix.empty()) {
        matrix = orientation_matrix(modifiers, axis_count);
    }

    std::vector<double> samples;
    const auto count = record.repeat * axis_count;
    samples.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const auto value = record.number(index);
        if (!value) {
            return std::nullopt;
        }
        samples.push_back(*value / divisors[index % divisors.size()]);
    }

    if (!matrix.empty()) {
        std::vector<double> transformed(axis_count);
        for (std::size_t index = 0; index < count; index += axis_count) {
            auto sample = std::span(samples).subspan(index, axis_count);
            for (std::size_t row = 0; row < axis_count; ++row) {
                transformed[row] = std::inner_product(
                    sample.begin(), sample.end(), matrix.data() + row * axis_count, 0.0);
            }
            std::ranges::copy(transformed, sample.begin());
        }
    }
    return samples;
}

} // namespace clst::gpmf
