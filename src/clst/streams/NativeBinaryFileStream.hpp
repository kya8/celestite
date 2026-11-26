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
    void open(Ex, const char* filename, FileStreamMode mode = FileStreamMode::Read);
    void open(Ex, const char* filename, FileStreamMode mode, FileAccessPattern pattern);

    // Take exclusive ownership of an existing handle at its current position.
    // Supply its access and append mode. Windows handles must support synchronous
    // file I/O and be closable with CloseHandle.
    // On failure, including when this stream is already open, ownership stays with the caller.
    [[nodiscard]] std::error_code adopt(NativeHandle handle, FileStreamMode mode) noexcept;
    void adopt(Ex, NativeHandle handle, FileStreamMode mode);

    // A flush failure leaves the stream open with the unwritten bytes available for retry.
    [[nodiscard]] std::error_code close() noexcept;
    void close(Ex);
    [[nodiscard]] bool is_open() const noexcept;

    // Synchronize the buffer, then give the caller ownership of the handle.
    // An empty stream returns nullptr on Windows or -1 on UNIX. If synchronization
    // fails, throw std::system_error and retain ownership.
    [[nodiscard]] NativeHandle release();

    // The stream owns this handle. Flush before using it for native I/O.
    [[nodiscard]] NativeHandle get_handle() const noexcept;

    // A transfer can report progress and an error together. A short transfer can also succeed.
    // Zero bytes without an error on a nonempty read denotes EOF.
    [[nodiscard]] TransferResult read(void* buffer, std::size_t size) noexcept;
    std::size_t read(Ex, void* buffer, std::size_t size);
    void read_exact(void* buffer, std::size_t size) { read(ex, buffer, size); }

    // A buffered write reports bytes accepted by the stream. A later flush can fail.
    [[nodiscard]] TransferResult write(const void* buffer, std::size_t size) noexcept;
    std::size_t write(Ex, const void* buffer, std::size_t size);
    void write_exact(const void* buffer, std::size_t size) { write(ex, buffer, size); }

    // Positioned transfers preserve the stream cursor on success.
    // Positioned writes require a non-append mode.
    [[nodiscard]] TransferResult read_at(OffsetType offset, void* buffer, std::size_t size) noexcept;
    std::size_t read_at(Ex, OffsetType offset, void* buffer, std::size_t size);
    [[nodiscard]] TransferResult write_at(OffsetType offset, const void* buffer, std::size_t size) noexcept;
    std::size_t write_at(Ex, OffsetType offset, const void* buffer, std::size_t size);

    // Drain pending writes or discard read-ahead, synchronizing the native cursor.
    [[nodiscard]] std::error_code flush() noexcept;
    void flush(Ex);

    // The default is 64 KiB. Zero disables buffering. Changing the size synchronizes
    // the current buffer when open and allocates the replacement on demand.
    [[nodiscard]] std::size_t get_buffer_size() const noexcept;
    [[nodiscard]] std::error_code set_buffer_size(std::size_t size) noexcept;
    void set_buffer_size(Ex, std::size_t size);

    // Submit buffered writes and request persistence on a writable stream.
    // Data may perform a full sync on some targets.
    [[nodiscard]] std::error_code sync(FileSyncMode mode = FileSyncMode::Full) noexcept;
    void sync(Ex, FileSyncMode mode = FileSyncMode::Full);

    [[nodiscard]] std::error_code seek(OffsetType offset, SeekFrom from = SeekFrom::Begin) noexcept;
    void seek(Ex, OffsetType offset, SeekFrom from = SeekFrom::Begin);
    [[nodiscard]] std::expected<OffsetType, std::error_code> tell() const noexcept;
    [[nodiscard]] OffsetType tell(Ex) const;
    [[nodiscard]] std::expected<OffsetType, std::error_code> get_length() const noexcept;
    [[nodiscard]] OffsetType get_length(Ex) const;

    // Change the logical EOF while preserving the cursor. Growth follows filesystem semantics.
    [[nodiscard]] std::error_code resize(OffsetType size) noexcept;
    void resize(Ex, OffsetType size);

    // Grow allocated capacity without changing EOF or the cursor. Sparse holes may remain.
    [[nodiscard]] std::error_code reserve(OffsetType size) noexcept;
    void reserve(Ex, OffsetType size);

    // Advise the OS to prefetch a byte range. An empty range is a no-op.
    // Nonempty ranges return operation_not_supported on Windows.
    [[nodiscard]] std::error_code prefetch(OffsetType offset, OffsetType size) noexcept;
    void prefetch(Ex, OffsetType offset, OffsetType size);

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
