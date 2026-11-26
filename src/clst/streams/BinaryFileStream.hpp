#ifndef CLST_STREAMS_BINARY_FILE_STREAM_HPP
#define CLST_STREAMS_BINARY_FILE_STREAM_HPP

#include "BinaryStream.hpp"
#include "FileStreamTypes.hpp"
#include <cstdio>
#include <expected>

namespace clst {

class BinaryFileStream : public RWStreamTag, public RWStreamMixin {
public:
    BinaryFileStream() noexcept = default;
    BinaryFileStream(const char* filename, FileStreamMode mode = FileStreamMode::Read);
    ~BinaryFileStream() noexcept;

    BinaryFileStream(const BinaryFileStream&) = delete;
    BinaryFileStream& operator=(const BinaryFileStream&) = delete;
    BinaryFileStream(BinaryFileStream&& rhs) noexcept;
    BinaryFileStream& operator=(BinaryFileStream&& rhs) noexcept;

    [[nodiscard]] std::error_code open(const char* filename, FileStreamMode mode = FileStreamMode::Read) noexcept;
    [[nodiscard]] std::error_code close() noexcept;
    [[nodiscard]] std::error_code flush() noexcept;
    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] std::expected<OffsetType, std::error_code> get_length() const noexcept;
    // Borrowed until close or an ownership transfer.
    [[nodiscard]] std::FILE* get_handle() const noexcept { return fp_; }

    // A nonempty read that returns zero bytes without an error has reached EOF.
    [[nodiscard]] TransferResult read(void* buf, std::size_t n) noexcept;
    [[nodiscard]] TransferResult write(const void* buf, std::size_t n) noexcept;
    [[nodiscard]] std::error_code seek(OffsetType offset, SeekFrom from = SeekFrom::Begin) noexcept;
    [[nodiscard]] std::expected<OffsetType, std::error_code> tell() const noexcept;

    void open_ex(const char* filename, FileStreamMode mode = FileStreamMode::Read);
    void close_ex();
    void flush_ex();
    std::size_t read_ex(void* buf, std::size_t n);
    std::size_t write_ex(const void* buf, std::size_t n);
    // Transfer exactly n bytes or throw std::system_error.
    void read_exact(void* buf, std::size_t n) { read_ex(buf, n); }
    void write_exact(const void* buf, std::size_t n) { write_ex(buf, n); }
    void seek_ex(OffsetType offset, SeekFrom from = SeekFrom::Begin);
    [[nodiscard]] OffsetType tell_ex() const;
    [[nodiscard]] OffsetType get_length_ex() const;

private:
    enum class Direction { None, Read, Write };

    std::FILE* fp_ = nullptr;
    FileStreamMode mode_ = FileStreamMode::Read;
    Direction direction_ = Direction::None;
};

} // namespace clst

#endif /* CLST_STREAMS_BINARY_FILE_STREAM_HPP */
