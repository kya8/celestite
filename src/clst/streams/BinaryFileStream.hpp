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
    // On failure the untagged constructor leaves the stream closed. Open reports the error code.
    BinaryFileStream(const char* filename, FileStreamMode mode = FileStreamMode::Read) noexcept;
    BinaryFileStream(Ex, const char* filename, FileStreamMode mode = FileStreamMode::Read);
    ~BinaryFileStream() noexcept;

    BinaryFileStream(const BinaryFileStream&) = delete;
    BinaryFileStream& operator=(const BinaryFileStream&) = delete;
    BinaryFileStream(BinaryFileStream&& rhs) noexcept;
    BinaryFileStream& operator=(BinaryFileStream&& rhs) noexcept;

    [[nodiscard]] std::error_code open(const char* filename, FileStreamMode mode = FileStreamMode::Read) noexcept;
    void open(Ex, const char* filename, FileStreamMode mode = FileStreamMode::Read);
    [[nodiscard]] std::error_code close() noexcept;
    void close(Ex);
    [[nodiscard]] std::error_code flush() noexcept;
    void flush(Ex);
    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] std::expected<OffsetType, std::error_code> get_length() const noexcept;
    [[nodiscard]] OffsetType get_length(Ex) const;
    // Borrowed until close or an ownership transfer.
    [[nodiscard]] std::FILE* get_handle() const noexcept { return fp_; }

    // A nonempty read that returns zero bytes without an error has reached EOF.
    [[nodiscard]] TransferResult read(void* buf, std::size_t n) noexcept;
    std::size_t read(Ex, void* buf, std::size_t n);
    [[nodiscard]] TransferResult write(const void* buf, std::size_t n) noexcept;
    std::size_t write(Ex, const void* buf, std::size_t n);
    // Retry interrupted calls and report progress if an exact transfer fails.
    [[nodiscard]] TransferResult read_exact(void* buf, std::size_t n) noexcept;
    [[nodiscard]] TransferResult write_exact(const void* buf, std::size_t n) noexcept;
    void read_exact(Ex, void* buf, std::size_t n) { read(ex, buf, n); }
    void write_exact(Ex, const void* buf, std::size_t n) { write(ex, buf, n); }

    [[nodiscard]] std::error_code seek(OffsetType offset, SeekFrom from = SeekFrom::Begin) noexcept;
    void seek(Ex, OffsetType offset, SeekFrom from = SeekFrom::Begin);
    [[nodiscard]] std::expected<OffsetType, std::error_code> tell() const noexcept;
    [[nodiscard]] OffsetType tell(Ex) const;

private:
    enum class Direction { None, Read, Write };

    std::FILE* fp_ = nullptr;
    FileStreamMode mode_ = FileStreamMode::Read;
    Direction direction_ = Direction::None;
};

} // namespace clst

#endif /* CLST_STREAMS_BINARY_FILE_STREAM_HPP */
