#ifndef CLST_STREAMS_NATIVE_BINARY_FILE_STREAM_HPP
#define CLST_STREAMS_NATIVE_BINARY_FILE_STREAM_HPP

#include "BinaryStream.hpp"
#include "FileStreamTypes.hpp"
#include <cstddef>
#include <expected>
#include <system_error>

namespace clst {

enum class FileAccessPattern { Normal, Sequential, Random };
enum class FileSyncMode { Full, Data };

struct AdoptHandleTag { explicit AdoptHandleTag() = default; };
inline constexpr AdoptHandleTag adopt_handle{};

// This class provides no internal thread synchronization. Callers must serialize
// concurrent use, including native I/O on its borrowed handle. Flush before
// changing the file through other handles.
class NativeBinaryFileStream : public RWStreamTag, public RWStreamMixin {
public:
#ifdef _WIN32
    using NativeHandle = void*;
#else
    using NativeHandle = int;
#endif

    NativeBinaryFileStream() noexcept;
    NativeBinaryFileStream(const char* filename, FileStreamMode mode = FileStreamMode::Read);
    NativeBinaryFileStream(const char* filename, FileStreamMode mode, FileAccessPattern pattern);
    NativeBinaryFileStream(AdoptHandleTag, NativeHandle handle, FileStreamMode mode);
    ~NativeBinaryFileStream() noexcept;

    NativeBinaryFileStream(const NativeBinaryFileStream&) = delete;
    NativeBinaryFileStream& operator=(const NativeBinaryFileStream&) = delete;
    NativeBinaryFileStream(NativeBinaryFileStream&& other) noexcept;
    NativeBinaryFileStream& operator=(NativeBinaryFileStream&& other) noexcept;

    [[nodiscard]] std::error_code open(const char* filename, FileStreamMode mode = FileStreamMode::Read) noexcept;
    // The access pattern is a best-effort hint to the operating system.
    [[nodiscard]] std::error_code open(const char* filename, FileStreamMode mode, FileAccessPattern pattern) noexcept;
    void open_ex(const char* filename, FileStreamMode mode = FileStreamMode::Read);
    void open_ex(const char* filename, FileStreamMode mode, FileAccessPattern pattern);

    // Take exclusive ownership of an existing handle at its current position.
    // Supply its access and append mode. Windows handles must support synchronous
    // file I/O and be closable with CloseHandle.
    // On failure, including when this stream is already open, ownership stays with the caller.
    [[nodiscard]] std::error_code adopt(NativeHandle handle, FileStreamMode mode) noexcept;
    void adopt_ex(NativeHandle handle, FileStreamMode mode);

    // A flush failure leaves the stream open with the unwritten bytes available for retry.
    [[nodiscard]] std::error_code close() noexcept;
    void close_ex();
    [[nodiscard]] bool is_open() const noexcept;

    // The stream owns this handle. Flush before using it for native I/O.
    [[nodiscard]] NativeHandle get_handle() const noexcept;

    // A transfer can report progress and an error together. A short transfer can also succeed.
    // Zero bytes without an error on a nonempty read denotes EOF.
    [[nodiscard]] TransferResult read(void* buffer, std::size_t size) noexcept;
    std::size_t read_ex(void* buffer, std::size_t size);

    // A buffered write reports bytes accepted by the stream. A later flush can fail.
    [[nodiscard]] TransferResult write(const void* buffer, std::size_t size) noexcept;
    std::size_t write_ex(const void* buffer, std::size_t size);

    // Positioned transfers preserve the stream cursor on success.
    // Positioned writes require a non-append mode.
    [[nodiscard]] TransferResult read_at(OffsetType offset, void* buffer, std::size_t size) noexcept;
    std::size_t read_at_ex(OffsetType offset, void* buffer, std::size_t size);
    [[nodiscard]] TransferResult write_at(OffsetType offset, const void* buffer, std::size_t size) noexcept;
    std::size_t write_at_ex(OffsetType offset, const void* buffer, std::size_t size);

    // Drain pending writes or discard read-ahead, synchronizing the native cursor.
    [[nodiscard]] std::error_code flush() noexcept;
    void flush_ex();

    // The default is 64 KiB. Zero disables buffering. Changing the size synchronizes
    // the current buffer when open and allocates the replacement on demand.
    [[nodiscard]] std::size_t get_buffer_size() const noexcept;
    [[nodiscard]] std::error_code set_buffer_size(std::size_t size) noexcept;
    void set_buffer_size_ex(std::size_t size);

    // Submit buffered writes and request persistence on a writable stream.
    // Data may perform a full sync on some targets.
    [[nodiscard]] std::error_code sync(FileSyncMode mode = FileSyncMode::Full) noexcept;
    void sync_ex(FileSyncMode mode = FileSyncMode::Full);

    [[nodiscard]] std::error_code seek(OffsetType offset, SeekFrom from = SeekFrom::Begin) noexcept;
    void seek_ex(OffsetType offset, SeekFrom from = SeekFrom::Begin);
    [[nodiscard]] std::expected<OffsetType, std::error_code> tell() const noexcept;
    [[nodiscard]] OffsetType tell_ex() const;
    [[nodiscard]] std::expected<OffsetType, std::error_code> get_length() const noexcept;
    [[nodiscard]] OffsetType get_length_ex() const;

    // Change the logical EOF while preserving the cursor. Growth follows filesystem semantics.
    [[nodiscard]] std::error_code resize(OffsetType size) noexcept;
    void resize_ex(OffsetType size);

    // Grow allocated capacity without changing EOF or the cursor. Sparse holes may remain.
    [[nodiscard]] std::error_code reserve(OffsetType size) noexcept;
    void reserve_ex(OffsetType size);

    // Advise the OS to prefetch a byte range. An empty range is a no-op.
    // Nonempty ranges return operation_not_supported on Windows.
    [[nodiscard]] std::error_code prefetch(OffsetType offset, OffsetType size) noexcept;
    void prefetch_ex(OffsetType offset, OffsetType size);

private:
    friend class WriteStreamMixin;
    struct Impl;

    std::size_t copy_from_native(NativeBinaryFileStream& source, std::size_t size);
    Impl& impl() noexcept;
    const Impl& impl() const noexcept;

    alignas(std::max_align_t) unsigned char storage_[64];
};

} // namespace clst

#endif /* CLST_STREAMS_NATIVE_BINARY_FILE_STREAM_HPP */
