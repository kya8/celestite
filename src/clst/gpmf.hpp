#ifndef CLST_GPMF_HPP
#define CLST_GPMF_HPP

#include "fourcc.hpp"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace clst::gpmf {

enum class ParseError {
    truncated_header,
    invalid_key,
    invalid_sample_size,
    truncated_value,
    invalid_container,
    nesting_limit,
    expected_device,
};

[[nodiscard]] std::string_view describe(ParseError error) noexcept;

// The value and any derived views borrow the buffer supplied to Reader.
struct RecordView {
    FourCC key;
    unsigned char type;
    std::uint8_t sample_size;
    std::uint16_t repeat;
    std::span<const unsigned char> value;

    [[nodiscard]] bool is_container() const noexcept { return type == 0; }
    [[nodiscard]] std::optional<std::string_view> text() const noexcept;
    [[nodiscard]] std::size_t numeric_elements_per_sample() const noexcept;
    // Converts an element to double. The index counts elements across samples in storage order.
    [[nodiscard]] std::optional<double> number(std::size_t index) const noexcept;
};

// Reads one level of KLV records. The input buffer must outlive the reader and its views.
class Reader {
public:
    explicit Reader(std::span<const unsigned char> bytes) noexcept : remaining_(bytes) {}

    // An empty optional marks end of input. After an error, discard this reader.
    [[nodiscard]] std::expected<std::optional<RecordView>, ParseError> next() noexcept;
    // The record must come from next() on this reader.
    [[nodiscard]] std::expected<Reader, ParseError> children(const RecordView& record) const noexcept;

private:
    std::span<const unsigned char> remaining_;
    std::size_t depth_ = 0;
};

struct Modifiers {
    std::optional<RecordView> scale;
    std::optional<RecordView> matrix;
    std::optional<RecordView> orientation_in;
    std::optional<RecordView> orientation_out;

    // Keep the first occurrence of each modifier in this level.
    void observe(const RecordView& record) noexcept;
};

namespace detail {

template<typename Visitor>
std::expected<void, ParseError> visit_records(Reader reader, Visitor& visitor, bool device_level)
{
    Modifiers modifiers;
    bool seen_record = false;
    for (;;) {
        const auto next = reader.next();
        if (!next) {
            return std::unexpected(next.error());
        }
        if (!*next) {
            if (device_level && !seen_record) {
                return std::unexpected(ParseError::expected_device);
            }
            return {};
        }

        const auto& record = **next;
        seen_record = true;
        if (device_level && (record.key != "DEVC"_fc || !record.is_container())) {
            return std::unexpected(ParseError::expected_device);
        }

        visitor(record, std::as_const(modifiers));
        modifiers.observe(record);
        if (record.is_container()) {
            const auto children = reader.children(record);
            if (!children) {
                return std::unexpected(children.error());
            }
            const auto result = visit_records(*children, visitor, false);
            if (!result) {
                return result;
            }
        }
    }
}

} // namespace detail

// Visits a payload's devices depth-first, with the preceding modifiers of each record.
template<typename Visitor>
[[nodiscard]] std::expected<void, ParseError> visit_payload(std::span<const unsigned char> bytes, Visitor&& visitor)
{
    return detail::visit_records(Reader(bytes), visitor, true);
}

// Returns scaled and oriented elements in sample order, or no value for an unsupported
// record or invalid modifier. Each sample must have exactly axis_count numeric elements.
[[nodiscard]] std::optional<std::vector<double>> decode_scaled_samples(
    const RecordView& record, const Modifiers& modifiers, std::size_t axis_count);

} // namespace clst::gpmf

#endif /* CLST_GPMF_HPP */
