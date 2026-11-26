#ifndef CLST_STREAMS_MEMORY_CURSOR_HPP
#define CLST_STREAMS_MEMORY_CURSOR_HPP

#include "BinaryStream.hpp"
#include <cstring>

namespace clst {

namespace detail {

template<bool Mutable>
struct Byte {
    using type = const unsigned char;
};
template<>
struct Byte<true> {
    using type = unsigned char;
};

template<bool Mutable>
using BytePointer = typename Byte<Mutable>::type*;

template<bool Mutable>
class CursorCommon {
protected:
    using Pointer = BytePointer<Mutable>;
public:
    // A null buffer or negative length constructs a closed cursor.
    CursorCommon(Pointer buf, std::ptrdiff_t len) noexcept
        : begin_(len >= 0 ? buf : nullptr), len_(begin_ ? len : 0) {}

    [[nodiscard]] bool is_open() const noexcept { return this->begin_ != nullptr; }
    [[nodiscard]] auto data() const noexcept { return this->begin_; }
    [[nodiscard]] auto len() const noexcept { return this->len_; }
    [[nodiscard]] OffsetType get_length() const noexcept { return len_; }
    static constexpr bool is_mutable = Mutable;

    [[nodiscard]] OffsetType tell() const noexcept { return position_; }
    [[nodiscard]] std::error_code seek(OffsetType offset, SeekFrom from = SeekFrom::Begin) noexcept
    {
        if (!is_open()) {
            return std::make_error_code(std::errc::bad_file_descriptor);
        }

        OffsetType base;
        switch (from) {
        case SeekFrom::Begin:   base = 0; break;
        case SeekFrom::Current: base = position_; break;
        case SeekFrom::End:     base = len_; break;
        default: return std::make_error_code(std::errc::invalid_argument);
        }
        if (offset < -base || offset > len_ - base) {
            return std::make_error_code(std::errc::result_out_of_range);
        }
        position_ = static_cast<std::ptrdiff_t>(base + offset);
        return {};
    }

    void seek(Ex, OffsetType offset, SeekFrom from = SeekFrom::Begin)
    {
        if (seek(offset, from)) {
            throw StreamIoError("Memory cursor seek failed");
        }
    }

    // Memory transfers are exact. A range error leaves the position unchanged.
    [[nodiscard]] TransferResult read(void* buf, std::size_t n) noexcept
    {
        if (const auto error = check_transfer(buf, n)) {
            return {0, error};
        }
        if (n != 0) {
            std::memmove(buf, begin_ + position_, n);
            position_ += static_cast<std::ptrdiff_t>(n);
        }
        return {n, {}};
    }

    void read(Ex, void* buf, std::size_t n)
    {
        if (read(buf, n).error) {
            throw StreamIoError("Memory cursor read failed");
        }
    }

    [[nodiscard]] TransferResult read_exact(void* buf, std::size_t n) noexcept { return read(buf, n); }
    void read_exact(Ex, void* buf, std::size_t n) { read(ex, buf, n); }

protected:
    std::error_code check_transfer(const void* buf, std::size_t n) const noexcept
    {
        if (!is_open()) {
            return std::make_error_code(std::errc::bad_file_descriptor);
        }
        if (n != 0 && !buf) {
            return std::make_error_code(std::errc::invalid_argument);
        }
        if (n > static_cast<std::size_t>(len_ - position_)) {
            return std::make_error_code(std::errc::result_out_of_range);
        }
        return {};
    }

    Pointer begin_ = nullptr;
    std::ptrdiff_t len_ = 0;
    std::ptrdiff_t position_ = 0;
};

} // namespace detail

template<bool Mutable>
class Cursor;

template<>
class Cursor<false> : public detail::CursorCommon<false>, public ReadStreamTag, public ReadStreamMixin {
public:
    Cursor(const void* buf, std::ptrdiff_t len) noexcept : detail::CursorCommon<false>(static_cast<Pointer>(buf), len) {}
};

template<>
class Cursor<true> : public detail::CursorCommon<true>, public RWStreamTag, public RWStreamMixin {
public:
    Cursor(void* buf, std::ptrdiff_t len) noexcept : detail::CursorCommon<true>(static_cast<Pointer>(buf), len) {}
    [[nodiscard]] TransferResult write(const void* buf, std::size_t n) noexcept
    {
        if (const auto error = check_transfer(buf, n)) {
            return {0, error};
        }
        if (n != 0) {
            std::memmove(begin_ + position_, buf, n);
            position_ += static_cast<std::ptrdiff_t>(n);
        }
        return {n, {}};
    }

    void write(Ex, const void* buf, std::size_t n)
    {
        if (write(buf, n).error) {
            throw StreamIoError("Memory cursor write failed");
        }
    }

    [[nodiscard]] TransferResult write_exact(const void* buf, std::size_t n) noexcept { return write(buf, n); }
    void write_exact(Ex, const void* buf, std::size_t n) { write(ex, buf, n); }
};

// CTAD guides
Cursor(const void*, std::ptrdiff_t) -> Cursor<false>;
Cursor(void*, std::ptrdiff_t) -> Cursor<true>;

} // namespace clst

#endif /* CLST_STREAMS_MEMORY_CURSOR_HPP */
