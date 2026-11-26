#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if !defined(_WIN32) && !defined(_FILE_OFFSET_BITS)
#define _FILE_OFFSET_BITS 64
#endif

#include "NativeBinaryFileStream.hpp"
#include "FileStreamOperations.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__linux__)
#include <optional>
#include <sys/syscall.h>
#endif
#endif

namespace clst {
namespace {

using NativeHandle = NativeBinaryFileStream::NativeHandle;
using OffsetResult = std::expected<OffsetType, std::error_code>;

struct PositionedTransfer {
    TransferResult result;
    bool cursor_preserved = true;
};

constexpr std::size_t default_buffer_size = 64 * 1024;

constexpr bool is_append(FileStreamMode mode) noexcept
{
    return mode == FileStreamMode::Append || mode == FileStreamMode::AppendExtended;
}

OffsetResult add_offset(OffsetType base, OffsetType offset) noexcept
{
    if (offset < -base)
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    if (offset > std::numeric_limits<OffsetType>::max() - base)
        return std::unexpected(std::make_error_code(std::errc::value_too_large));
    return base + offset;
}

std::error_code check_transfer_range(OffsetType offset, const void* buffer, std::size_t size) noexcept
{
    if (offset < 0 || (size != 0 && !buffer))
        return std::make_error_code(std::errc::invalid_argument);
    if (std::cmp_greater(size, std::numeric_limits<OffsetType>::max() - offset))
        return std::make_error_code(std::errc::value_too_large);
    return {};
}

#ifdef _WIN32

constexpr NativeHandle closed_handle = nullptr;

std::error_code win32_error() noexcept
{
    return {static_cast<int>(GetLastError()), std::system_category()};
}

std::expected<NativeHandle, std::error_code>
native_open(const char* filename, FileStreamMode mode, FileAccessPattern pattern) noexcept
{
    DWORD access;
    DWORD creation;
    switch (mode) {
    case FileStreamMode::Read:
        access = GENERIC_READ;
        creation = OPEN_EXISTING;
        break;
    case FileStreamMode::Write:
        access = GENERIC_WRITE | FILE_READ_ATTRIBUTES;
        creation = CREATE_ALWAYS;
        break;
    case FileStreamMode::Append:
        access = GENERIC_WRITE | FILE_READ_ATTRIBUTES;
        creation = OPEN_ALWAYS;
        break;
    case FileStreamMode::ReadExtended:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = OPEN_EXISTING;
        break;
    case FileStreamMode::WriteExtended:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = CREATE_ALWAYS;
        break;
    case FileStreamMode::AppendExtended:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = OPEN_ALWAYS;
        break;
    default:
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }

    DWORD flags = FILE_ATTRIBUTE_NORMAL;
    switch (pattern) {
    case FileAccessPattern::Normal: break;
    case FileAccessPattern::Sequential: flags |= FILE_FLAG_SEQUENTIAL_SCAN; break;
    case FileAccessPattern::Random: flags |= FILE_FLAG_RANDOM_ACCESS; break;
    default:
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }

    auto handle = CreateFileA(filename, access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, creation, flags, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return std::unexpected(win32_error());
    return handle;
}

std::error_code validate_adopted_handle(NativeHandle handle, FileStreamMode mode) noexcept
{
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
        return std::make_error_code(std::errc::bad_file_descriptor);

    switch (mode) {
    case FileStreamMode::Read:
    case FileStreamMode::Write:
    case FileStreamMode::Append:
    case FileStreamMode::ReadExtended:
    case FileStreamMode::WriteExtended:
    case FileStreamMode::AppendExtended:
        break;
    default:
        return std::make_error_code(std::errc::invalid_argument);
    }

    // FILE_TYPE_UNKNOWN can also be a successful result.
    SetLastError(ERROR_SUCCESS);
    if (GetFileType(handle) == FILE_TYPE_UNKNOWN)
        return win32_error();
    return {};
}

std::error_code native_close(NativeHandle handle) noexcept
{
    if (CloseHandle(handle))
        return {};
    return win32_error();
}

bool is_regular_file(NativeHandle handle) noexcept
{
    return GetFileType(handle) == FILE_TYPE_DISK;
}

OffsetResult native_length(NativeHandle handle) noexcept
{
    LARGE_INTEGER length;
    if (!GetFileSizeEx(handle, &length))
        return std::unexpected(win32_error());
    return length.QuadPart;
}

OffsetResult native_seek(NativeHandle handle, OffsetType offset, SeekFrom from) noexcept
{
    DWORD origin;
    switch (from) {
    case SeekFrom::Begin: origin = FILE_BEGIN; break;
    case SeekFrom::Current: origin = FILE_CURRENT; break;
    case SeekFrom::End: origin = FILE_END; break;
    default:
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }

    LARGE_INTEGER distance;
    distance.QuadPart = offset;
    LARGE_INTEGER position;
    if (!SetFilePointerEx(handle, distance, &position, origin))
        return std::unexpected(win32_error());
    return position.QuadPart;
}

TransferResult native_read(NativeHandle handle, void* buffer, std::size_t size) noexcept
{
    const auto request = static_cast<DWORD>(std::min<std::size_t>(size, std::numeric_limits<DWORD>::max()));
    DWORD count = 0;
    if (!ReadFile(handle, buffer, request, &count, nullptr)) {
        const auto error = GetLastError();
        if (error != ERROR_HANDLE_EOF && error != ERROR_BROKEN_PIPE)
            return {count, {static_cast<int>(error), std::system_category()}};
    }
    return {count, {}};
}

TransferResult native_write(NativeHandle handle, const void* buffer, std::size_t size, bool append) noexcept
{
    const auto request = static_cast<DWORD>(std::min<std::size_t>(size, std::numeric_limits<DWORD>::max()));
    OVERLAPPED append_offset{};
    // These offsets request an append through the synchronous handle.
    append_offset.Offset = std::numeric_limits<DWORD>::max();
    append_offset.OffsetHigh = std::numeric_limits<DWORD>::max();
    DWORD count = 0;
    if (!WriteFile(handle, buffer, request, &count, append ? &append_offset : nullptr))
        return {count, win32_error()};
    return {count, {}};
}

template<typename Transfer>
PositionedTransfer native_transfer_at(NativeHandle handle, OffsetType offset, Transfer transfer) noexcept
{
    const auto previous = native_seek(handle, 0, SeekFrom::Current);
    if (!previous)
        return {{0, previous.error()}};
    const auto positioned = native_seek(handle, offset, SeekFrom::Begin);
    if (!positioned)
        return {{0, positioned.error()}};

    auto result = transfer();
    const auto restored = native_seek(handle, *previous, SeekFrom::Begin);
    if (!restored && !result.error)
        result.error = restored.error();
    return {result, restored.has_value()};
}

PositionedTransfer native_read_at(NativeHandle handle, OffsetType offset, void* buffer, std::size_t size) noexcept
{
    return native_transfer_at(handle, offset, [=] { return native_read(handle, buffer, size); });
}

PositionedTransfer native_write_at(NativeHandle handle, OffsetType offset, const void* buffer, std::size_t size) noexcept
{
    return native_transfer_at(handle, offset, [=] { return native_write(handle, buffer, size, false); });
}

std::error_code native_resize(NativeHandle handle, OffsetType size) noexcept
{
    FILE_END_OF_FILE_INFO info{};
    info.EndOfFile.QuadPart = size;
    if (!SetFileInformationByHandle(handle, FileEndOfFileInfo, &info, static_cast<DWORD>(sizeof(info))))
        return win32_error();
    return {};
}

std::error_code native_reserve(NativeHandle handle, OffsetType size) noexcept
{
    FILE_STANDARD_INFO current{};
    if (!GetFileInformationByHandleEx(handle, FileStandardInfo, &current, static_cast<DWORD>(sizeof(current))))
        return win32_error();
    if (size <= current.AllocationSize.QuadPart)
        return {};
    // A sparse file can have a smaller allocation than its EOF. A request in
    // that interval cannot be passed through because it could truncate the file.
    if (size < current.EndOfFile.QuadPart)
        return std::make_error_code(std::errc::operation_not_supported);

    FILE_ALLOCATION_INFO info{};
    info.AllocationSize.QuadPart = size;
    if (!SetFileInformationByHandle(handle, FileAllocationInfo, &info, static_cast<DWORD>(sizeof(info))))
        return win32_error();
    return {};
}

std::error_code native_sync(NativeHandle handle, FileSyncMode) noexcept
{
    if (!FlushFileBuffers(handle))
        return win32_error();
    return {};
}

std::error_code native_prefetch(NativeHandle, OffsetType, OffsetType) noexcept
{
    return std::make_error_code(std::errc::operation_not_supported);
}

#else

constexpr NativeHandle closed_handle = -1;
static_assert(std::numeric_limits<off_t>::digits >= std::numeric_limits<OffsetType>::digits);

std::error_code posix_error() noexcept
{
    return {errno, std::generic_category()};
}

template<typename Operation>
std::error_code posix_status(Operation operation) noexcept
{
    int result;
    do {
        result = operation();
    } while (result == -1 && errno == EINTR);
    return result == -1 ? posix_error() : std::error_code{};
}

template<typename Operation>
TransferResult posix_transfer(std::size_t size, Operation operation) noexcept
{
    const auto request = std::min<std::size_t>(size, std::numeric_limits<ssize_t>::max());
    ssize_t count;
    do {
        count = operation(request);
    } while (count == -1 && errno == EINTR);
    if (count == -1)
        return {0, posix_error()};
    return {static_cast<std::size_t>(count), {}};
}

bool is_regular_file(NativeHandle handle) noexcept
{
    struct stat info;
    return ::fstat(handle, &info) == 0 && S_ISREG(info.st_mode);
}

void advise_access(NativeHandle handle, FileAccessPattern pattern) noexcept
{
    if (pattern == FileAccessPattern::Normal || !is_regular_file(handle))
        return;

#if defined(__APPLE__)
    (void)::fcntl(handle, F_RDAHEAD, pattern == FileAccessPattern::Sequential ? 1 : 0);
#elif defined(POSIX_FADV_SEQUENTIAL) && defined(POSIX_FADV_RANDOM)
    const int advice = pattern == FileAccessPattern::Sequential ? POSIX_FADV_SEQUENTIAL : POSIX_FADV_RANDOM;
    (void)::posix_fadvise(handle, 0, 0, advice);
#endif
}

int posix_open_flags(FileStreamMode mode) noexcept
{
    switch (mode) {
    case FileStreamMode::Read:           return O_RDONLY;
    case FileStreamMode::Write:          return O_WRONLY | O_CREAT | O_TRUNC;
    case FileStreamMode::Append:         return O_WRONLY | O_CREAT | O_APPEND;
    case FileStreamMode::ReadExtended:   return O_RDWR;
    case FileStreamMode::WriteExtended:  return O_RDWR | O_CREAT | O_TRUNC;
    case FileStreamMode::AppendExtended: return O_RDWR | O_CREAT | O_APPEND;
    }
    return -1;
}

std::expected<NativeHandle, std::error_code>
native_open(const char* filename, FileStreamMode mode, FileAccessPattern pattern) noexcept
{
    const auto flags = posix_open_flags(mode);
    if (flags == -1)
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));

    NativeHandle handle;
    do {
        handle = ::open(filename, flags | O_CLOEXEC, 0666);
    } while (handle == closed_handle && errno == EINTR);
    if (handle == closed_handle)
        return std::unexpected(posix_error());
    advise_access(handle, pattern);
    return handle;
}

std::error_code validate_adopted_handle(NativeHandle handle, FileStreamMode mode) noexcept
{
    int flags;
    do {
        flags = ::fcntl(handle, F_GETFL);
    } while (flags == -1 && errno == EINTR);
    if (flags == -1)
        return posix_error();
#ifdef O_PATH
    if ((flags & O_PATH) != 0)
        return std::make_error_code(std::errc::bad_file_descriptor);
#endif

    constexpr int mode_mask = O_ACCMODE | O_APPEND;
    const auto expected = posix_open_flags(mode);
    if (expected == -1 || (flags & mode_mask) != (expected & mode_mask))
        return std::make_error_code(std::errc::invalid_argument);
    return {};
}

std::error_code native_close(NativeHandle handle) noexcept
{
    // Retrying a failed close can target a descriptor already reused by another thread.
    if (::close(handle) == 0)
        return {};
    return posix_error();
}

OffsetResult native_length(NativeHandle handle) noexcept
{
    struct stat info;
    if (::fstat(handle, &info) == -1)
        return std::unexpected(posix_error());
    return static_cast<OffsetType>(info.st_size);
}

OffsetResult native_seek(NativeHandle handle, OffsetType offset, SeekFrom from) noexcept
{
    int origin;
    switch (from) {
    case SeekFrom::Begin: origin = SEEK_SET; break;
    case SeekFrom::Current: origin = SEEK_CUR; break;
    case SeekFrom::End: origin = SEEK_END; break;
    default: return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }

    const auto position = ::lseek(handle, static_cast<off_t>(offset), origin);
    if (position == -1)
        return std::unexpected(posix_error());
    return static_cast<OffsetType>(position);
}

TransferResult native_read(NativeHandle handle, void* buffer, std::size_t size) noexcept
{
    return posix_transfer(size, [=](std::size_t request) { return ::read(handle, buffer, request); });
}

TransferResult native_write(NativeHandle handle, const void* buffer, std::size_t size, bool) noexcept
{
    return posix_transfer(size, [=](std::size_t request) { return ::write(handle, buffer, request); });
}

PositionedTransfer native_read_at(NativeHandle handle, OffsetType offset, void* buffer, std::size_t size) noexcept
{
    return {posix_transfer(size, [=](std::size_t request) {
        return ::pread(handle, buffer, request, static_cast<off_t>(offset));
    })};
}

PositionedTransfer native_write_at(NativeHandle handle, OffsetType offset, const void* buffer, std::size_t size) noexcept
{
    return {posix_transfer(size, [=](std::size_t request) {
        return ::pwrite(handle, buffer, request, static_cast<off_t>(offset));
    })};
}

std::error_code native_resize(NativeHandle handle, OffsetType length) noexcept
{
    return posix_status([=] { return ::ftruncate(handle, static_cast<off_t>(length)); });
}

std::error_code native_reserve(NativeHandle handle, OffsetType capacity) noexcept
{
#if defined(__linux__)
    return posix_status([=] { return ::fallocate(handle, FALLOC_FL_KEEP_SIZE, 0, static_cast<off_t>(capacity)); });
#elif defined(__APPLE__)
    struct stat info;
    if (::fstat(handle, &info) == -1)
        return posix_error();

    // Darwin reports allocated storage in 512-byte units. F_PEOFPOSMODE adds
    // new space beyond the physical EOF, so request only the missing capacity.
    constexpr OffsetType block_size = 512;
    const auto required_blocks = capacity / block_size + (capacity % block_size != 0);
    const auto allocated_blocks = std::max(info.st_blocks, static_cast<blkcnt_t>(0));
    if (allocated_blocks >= required_blocks)
        return {};
    const auto additional = static_cast<off_t>(capacity - static_cast<OffsetType>(allocated_blocks) * block_size);
    fstore_t allocation{F_ALLOCATEALL, F_PEOFPOSMODE, 0, additional, 0};
    if (const auto error = posix_status([&] { return ::fcntl(handle, F_PREALLOCATE, &allocation); }))
        return error;
    if (allocation.fst_bytesalloc < additional)
        return std::make_error_code(std::errc::no_space_on_device);
    return {};
#else
    (void)handle;
    (void)capacity;
    return std::make_error_code(std::errc::operation_not_supported);
#endif
}

std::error_code native_sync(NativeHandle handle, FileSyncMode mode) noexcept
{
#if defined(__APPLE__)
    if (mode == FileSyncMode::Full) {
        const auto error = posix_status([=] { return ::fcntl(handle, F_FULLFSYNC); });
        if (!error)
            return {};
        if (error != std::errc::not_supported && error != std::errc::operation_not_supported &&
            error != std::errc::invalid_argument && error != std::errc::inappropriate_io_control_operation)
            return error;
    }
#elif defined(__linux__)
    if (mode == FileSyncMode::Data)
        return posix_status([=] { return ::fdatasync(handle); });
#else
    (void)mode;
#endif
    return posix_status([=] { return ::fsync(handle); });
}

std::error_code native_prefetch(NativeHandle handle, OffsetType offset, OffsetType length) noexcept
{
#if defined(__APPLE__)
    const auto file_size = native_length(handle);
    if (!file_size)
        return file_size.error();
    if (offset >= *file_size)
        return {};

    auto remaining = std::min(length, *file_size - offset);
    while (remaining > 0) {
        const auto request = std::min<OffsetType>(remaining, std::numeric_limits<int>::max());
        struct radvisory range{static_cast<off_t>(offset), static_cast<int>(request)};
        if (const auto error = posix_status([&] { return ::fcntl(handle, F_RDADVISE, &range); }))
            return error;
        offset += request;
        remaining -= request;
    }
    return {};
#elif defined(POSIX_FADV_WILLNEED)
    return {::posix_fadvise(handle, static_cast<off_t>(offset), static_cast<off_t>(length), POSIX_FADV_WILLNEED),
            std::generic_category()};
#else
    (void)handle;
    (void)offset;
    (void)length;
    return std::make_error_code(std::errc::operation_not_supported);
#endif
}

#if defined(__linux__)

#if defined(SYS_copy_file_range)
long copy_range_syscall(NativeHandle source, NativeHandle destination, std::size_t size) noexcept
{
    long result;
    do {
        result = ::syscall(SYS_copy_file_range, source, nullptr, destination, nullptr, size, 0u);
    } while (result == -1 && errno == EINTR);
    return result;
}
#endif

#if defined(SYS_copy_file_range)
const bool native_range_copy_available = [] {
    const auto saved_errno = errno;
    // EBADF confirms the call reached the kernel's descriptor checks.
    const bool available = copy_range_syscall(-1, -1, 1) == -1 && errno == EBADF;
    errno = saved_errno;
    return available;
}();
#else
constexpr bool native_range_copy_available = false;
#endif

bool native_range_copy_same_filesystem(NativeHandle source, NativeHandle destination) noexcept
{
    // Older kernels could report a successful cross-filesystem copy without moving any data.
    struct stat source_info;
    struct stat destination_info;
    return ::fstat(source, &source_info) == 0 &&
           ::fstat(destination, &destination_info) == 0 &&
           source_info.st_dev == destination_info.st_dev;
}

std::optional<TransferResult> native_copy_range(NativeHandle source, NativeHandle destination, std::size_t size) noexcept
{
#if defined(SYS_copy_file_range)
    const auto request = std::min<std::size_t>(size, std::numeric_limits<ssize_t>::max());
    const auto result = copy_range_syscall(source, destination, request);
    if (result >= 0)
        return TransferResult{static_cast<std::size_t>(result), {}};

    const auto error = posix_error();
    if (error.value() == ENOSYS || error.value() == EXDEV || error.value() == EOPNOTSUPP)
        return std::nullopt;
    return TransferResult{0, error};
#else
    (void)source;
    (void)destination;
    (void)size;
    return std::nullopt;
#endif
}

#endif // __linux__
#endif

} // namespace

struct NativeBinaryFileStream::Impl {
    enum class CacheState { Empty, Reading, Writing };

    NativeHandle handle = closed_handle;
    FileStreamMode mode = FileStreamMode::Read;
    bool regular_file = false;
    CacheState state = CacheState::Empty;
    std::unique_ptr<unsigned char[]> cache;
    OffsetType cache_begin = 0;
    std::size_t cache_size = 0;
    std::size_t read_offset = 0;
    std::size_t buffer_capacity = default_buffer_size;

    TransferResult read(void* buffer, std::size_t size, bool direct) noexcept;
    TransferResult write(const void* buffer, std::size_t size, bool direct) noexcept;

    bool append() const noexcept { return is_append(mode); }
    bool readable() const noexcept
    {
        return handle != closed_handle && mode != FileStreamMode::Write && mode != FileStreamMode::Append;
    }
    bool writable() const noexcept
    {
        return handle != closed_handle && mode != FileStreamMode::Read;
    }

    bool ensure_cache() noexcept
    {
        if (!cache)
            cache.reset(new (std::nothrow) unsigned char[buffer_capacity]);
        return cache != nullptr;
    }

    // Reading leaves the OS cursor at the end of the read-ahead window. Writing leaves
    // it at the start of the pending bytes. An empty cache shares the native cursor.
    OffsetResult position() const noexcept
    {
        switch (state) {
        case CacheState::Reading:
            return add_offset(cache_begin, static_cast<OffsetType>(read_offset));
        case CacheState::Writing:
            if (append())
                return length();
            return add_offset(cache_begin, static_cast<OffsetType>(cache_size));
        case CacheState::Empty:
            return native_seek(handle, 0, SeekFrom::Current);
        }
        std::unreachable();
    }

    OffsetResult length() const noexcept
    {
        auto length = native_length(handle);
        if (!length || state != CacheState::Writing)
            return length;

        const auto pending_size = static_cast<OffsetType>(cache_size);
        if (append())
            return add_offset(*length, pending_size);

        const auto pending_end = add_offset(cache_begin, pending_size);
        if (!pending_end)
            return pending_end;
        return std::max(*length, *pending_end);
    }

    std::error_code flush_writes() noexcept
    {
        if (state != CacheState::Writing)
            return {};

        std::size_t committed = 0;
        std::error_code error;
        while (committed < cache_size) {
            const auto result = native_write(handle, cache.get() + committed, cache_size - committed, append());
            committed += result.bytes_transferred;
            error = result.error;
            if (error)
                break;
            if (result.bytes_transferred == 0) {
                error = std::make_error_code(std::errc::io_error);
                break;
            }
        }

        cache_size -= committed;
        if (cache_size == 0) {
            state = CacheState::Empty;
        } else if (committed != 0) {
            std::memmove(cache.get(), cache.get() + committed, cache_size);
            if (!append())
                cache_begin += static_cast<OffsetType>(committed);
        }
        return error;
    }

    std::error_code synchronize() noexcept
    {
        if (state != CacheState::Reading)
            return flush_writes();

        const auto logical_position = position();
        if (!logical_position)
            return logical_position.error();
        const auto result = native_seek(handle, *logical_position, SeekFrom::Begin);
        if (!result)
            return result.error();
        state = CacheState::Empty;
        return {};
    }

    std::size_t copy_from_cache(void* destination, std::size_t size) noexcept
    {
        const auto copied = std::min(size, cache_size - read_offset);
        std::memcpy(destination, cache.get() + read_offset, copied);
        read_offset += copied;
        if (read_offset == cache_size)
            state = CacheState::Empty;
        return copied;
    }

    bool read_cached_range(OffsetType offset, void* destination, std::size_t size) const noexcept
    {
        if (state == CacheState::Empty)
            return false;

        auto begin = cache_begin;
        if (state == CacheState::Writing && append()) {
            const auto end = native_length(handle);
            if (!end)
                return false;
            begin = *end;
        }
        if (offset < begin || std::cmp_greater_equal(offset - begin, cache_size))
            return false;

        const auto relative = static_cast<std::size_t>(offset - begin);
        if (size > cache_size - relative)
            return false;

        std::memcpy(destination, cache.get() + relative, size);
        return true;
    }

    bool overlaps_read_ahead(OffsetType offset, std::size_t size) const noexcept
    {
        if (state != CacheState::Reading)
            return false;
        if (offset < cache_begin)
            return std::cmp_greater(size, cache_begin - offset);
        return std::cmp_less(offset - cache_begin, cache_size);
    }

    TransferResult finish_positioned_transfer(PositionedTransfer transfer) noexcept
    {
        if (!transfer.cursor_preserved && state == CacheState::Reading) {
            // A failed Win32 restoration may have left the native cursor outside the cache.
            (void)synchronize();
            state = CacheState::Empty;
        }
        return transfer.result;
    }

    NativeHandle detach_handle() noexcept
    {
        state = CacheState::Empty;
        cache.reset();
        return std::exchange(handle, closed_handle);
    }

    std::error_code close_handle() noexcept
    {
        return native_close(detach_handle());
    }

    // Called when destruction or a noexcept move must release ownership even on error.
    void close_best_effort() noexcept
    {
        if (handle != closed_handle) {
            (void)flush_writes();
            (void)close_handle();
        }
    }
};

NativeBinaryFileStream::NativeBinaryFileStream() noexcept
{
    static_assert(sizeof(Impl) <= sizeof(storage_));
    static_assert(alignof(Impl) <= alignof(std::max_align_t));
    ::new (static_cast<void*>(storage_)) Impl;
}

NativeBinaryFileStream::NativeBinaryFileStream(const char* filename, FileStreamMode mode)
    : NativeBinaryFileStream(filename, mode, FileAccessPattern::Normal)
{
}

NativeBinaryFileStream::NativeBinaryFileStream(const char* filename, FileStreamMode mode, FileAccessPattern pattern)
    : NativeBinaryFileStream()
{
    open(ex, filename, mode, pattern);
}

NativeBinaryFileStream::NativeBinaryFileStream(AdoptHandleTag, NativeHandle handle, FileStreamMode mode)
    : NativeBinaryFileStream()
{
    adopt(ex, handle, mode);
}

NativeBinaryFileStream::~NativeBinaryFileStream() noexcept
{
    impl().close_best_effort();
    std::destroy_at(&impl());
}

NativeBinaryFileStream::NativeBinaryFileStream(NativeBinaryFileStream&& other) noexcept
    : NativeBinaryFileStream()
{
    *this = std::move(other);
}

NativeBinaryFileStream& NativeBinaryFileStream::operator=(NativeBinaryFileStream&& other) noexcept
{
    if (this != &other) {
        impl().close_best_effort();
        impl() = std::exchange(other.impl(), Impl{});
    }
    return *this;
}

NativeBinaryFileStream::Impl& NativeBinaryFileStream::impl() noexcept
{
    return *std::launder(reinterpret_cast<Impl*>(storage_));
}

const NativeBinaryFileStream::Impl& NativeBinaryFileStream::impl() const noexcept
{
    return *std::launder(reinterpret_cast<const Impl*>(storage_));
}

std::error_code NativeBinaryFileStream::open(const char* filename, FileStreamMode mode) noexcept
{
    return open(filename, mode, FileAccessPattern::Normal);
}

std::error_code NativeBinaryFileStream::open(const char* filename, FileStreamMode mode, FileAccessPattern pattern) noexcept
{
    if (is_open())
        return std::make_error_code(std::errc::device_or_resource_busy);
    if (!filename)
        return std::make_error_code(std::errc::invalid_argument);
    if (pattern != FileAccessPattern::Normal && pattern != FileAccessPattern::Sequential && pattern != FileAccessPattern::Random)
        return std::make_error_code(std::errc::invalid_argument);

    const auto result = native_open(filename, mode, pattern);
    if (!result)
        return result.error();

    const auto handle = *result;
    const auto regular_file = is_regular_file(handle);
    if (regular_file && is_append(mode)) {
        const auto position = native_seek(handle, 0, SeekFrom::End);
        if (!position) {
            (void)native_close(handle);
            return position.error();
        }
    }

    auto& state = impl();
    state.handle = handle;
    state.mode = mode;
    state.regular_file = regular_file;
    return {};
}

void NativeBinaryFileStream::open(Ex, const char* filename, FileStreamMode mode)
{
    detail::throw_file_error(open(filename, mode), "Open file");
}

void NativeBinaryFileStream::open(Ex, const char* filename, FileStreamMode mode, FileAccessPattern pattern)
{
    detail::throw_file_error(open(filename, mode, pattern), "Open file");
}

std::error_code NativeBinaryFileStream::adopt(NativeHandle handle, FileStreamMode mode) noexcept
{
    if (is_open())
        return std::make_error_code(std::errc::device_or_resource_busy);

    if (const auto error = validate_adopted_handle(handle, mode))
        return error;

    auto& state = impl();
    state.handle = handle;
    state.mode = mode;
    state.regular_file = is_regular_file(handle);
    return {};
}

void NativeBinaryFileStream::adopt(Ex, NativeHandle handle, FileStreamMode mode)
{
    detail::throw_file_error(adopt(handle, mode), "Adopt file handle");
}

std::error_code NativeBinaryFileStream::close() noexcept
{
    if (!is_open())
        return {};
    auto& state = impl();
    if (const auto error = state.flush_writes())
        return error;
    return state.close_handle();
}

void NativeBinaryFileStream::close(Ex)
{
    detail::throw_file_error(close(), "Close file");
}

bool NativeBinaryFileStream::is_open() const noexcept
{
    return get_handle() != closed_handle;
}

NativeBinaryFileStream::NativeHandle NativeBinaryFileStream::release()
{
    if (!is_open())
        return closed_handle;

    auto& state = impl();
    detail::throw_file_error(state.synchronize(), "Release file handle");
    return state.detach_handle();
}

NativeBinaryFileStream::NativeHandle NativeBinaryFileStream::get_handle() const noexcept
{
    return impl().handle;
}

TransferResult NativeBinaryFileStream::Impl::read(void* buffer, std::size_t size, bool direct) noexcept
{
    if (!readable())
        return {0, std::make_error_code(std::errc::bad_file_descriptor)};
    if (size == 0)
        return {};
    if (!buffer)
        return {0, std::make_error_code(std::errc::invalid_argument)};

    if (!regular_file)
        return native_read(handle, buffer, size);
    if (const auto error = flush_writes())
        return {0, error};

    std::size_t copied = 0;
    if (state == CacheState::Reading) {
        copied = copy_from_cache(buffer, size);
        if (copied == size)
            return {copied, {}};
    }

    auto* remaining = static_cast<unsigned char*>(buffer) + copied;
    if (direct || !ensure_cache()) {
        auto result = native_read(handle, remaining, size - copied);
        result.bytes_transferred += copied;
        return result;
    }

    const auto begin = position();
    if (!begin)
        return {copied, begin.error()};
    const auto result = native_read(handle, cache.get(), buffer_capacity);
    if (result.bytes_transferred != 0) {
        cache_begin = *begin;
        cache_size = result.bytes_transferred;
        read_offset = 0;
        state = CacheState::Reading;
        copied += copy_from_cache(remaining, size - copied);
    }
    return {copied, result.error};
}

TransferResult NativeBinaryFileStream::read(void* buffer, std::size_t size) noexcept
{
    auto& state = impl();
    return state.read(buffer, size, size >= state.buffer_capacity);
}

std::size_t NativeBinaryFileStream::read(Ex, void* buffer, std::size_t size)
{
    auto& state = impl();
    return detail::read_file_exact(buffer, size, [&state, direct = size >= state.buffer_capacity](void* data, std::size_t n) {
        return state.read(data, n, direct);
    });
}

TransferResult NativeBinaryFileStream::Impl::write(const void* buffer, std::size_t size, bool direct) noexcept
{
    if (!writable())
        return {0, std::make_error_code(std::errc::bad_file_descriptor)};
    if (size == 0)
        return {};
    if (!buffer)
        return {0, std::make_error_code(std::errc::invalid_argument)};

    if (direct || !regular_file || !ensure_cache()) {
        if (const auto error = synchronize())
            return {0, error};
        return native_write(handle, buffer, size, append());
    }

    if (state != CacheState::Writing || size > buffer_capacity - cache_size) {
        if (const auto error = synchronize())
            return {0, error};
    }
    if (state == CacheState::Empty) {
        if (!append()) {
            const auto begin = position();
            if (!begin)
                return {0, begin.error()};
            cache_begin = *begin;
        }
        cache_size = 0;
    }
    if (!append()) {
        const auto end = add_offset(cache_begin, static_cast<OffsetType>(cache_size + size));
        if (!end)
            return {0, end.error()};
    }

    std::memcpy(cache.get() + cache_size, buffer, size);
    cache_size += size;
    state = CacheState::Writing;
    return {size, {}};
}

TransferResult NativeBinaryFileStream::write(const void* buffer, std::size_t size) noexcept
{
    auto& state = impl();
    return state.write(buffer, size, size >= state.buffer_capacity);
}

std::size_t NativeBinaryFileStream::write(Ex, const void* buffer, std::size_t size)
{
    auto& state = impl();
    return detail::write_file_exact(buffer, size, [&state, direct = size >= state.buffer_capacity](const void* data, std::size_t n) {
        return state.write(data, n, direct);
    });
}

TransferResult NativeBinaryFileStream::read_at(OffsetType offset, void* buffer, std::size_t size) noexcept
{
    auto& state = impl();
    if (!state.readable())
        return {0, std::make_error_code(std::errc::bad_file_descriptor)};
    if (const auto error = check_transfer_range(offset, buffer, size))
        return {0, error};
    if (size == 0)
        return {};

    if (state.read_cached_range(offset, buffer, size))
        return {size, {}};
    if (const auto error = state.flush_writes())
        return {0, error};
    return state.finish_positioned_transfer(native_read_at(state.handle, offset, buffer, size));
}

std::size_t NativeBinaryFileStream::read_at(Ex, OffsetType offset, void* buffer, std::size_t size)
{
    return detail::read_file_exact(buffer, size, [this, &offset](void* data, std::size_t n) {
        const auto result = read_at(offset, data, n);
        offset += static_cast<OffsetType>(result.bytes_transferred);
        return result;
    });
}

TransferResult NativeBinaryFileStream::write_at(OffsetType offset, const void* buffer, std::size_t size) noexcept
{
    auto& state = impl();
    if (!state.writable())
        return {0, std::make_error_code(std::errc::bad_file_descriptor)};
    if (state.append())
        return {0, std::make_error_code(std::errc::operation_not_supported)};
    if (const auto error = check_transfer_range(offset, buffer, size))
        return {0, error};
    if (size == 0)
        return {};

    const auto error = state.overlaps_read_ahead(offset, size) ? state.synchronize() : state.flush_writes();
    if (error)
        return {0, error};
    return state.finish_positioned_transfer(native_write_at(state.handle, offset, buffer, size));
}

std::size_t NativeBinaryFileStream::write_at(Ex, OffsetType offset, const void* buffer, std::size_t size)
{
    return detail::write_file_exact(buffer, size, [this, &offset](const void* data, std::size_t n) {
        const auto result = write_at(offset, data, n);
        offset += static_cast<OffsetType>(result.bytes_transferred);
        return result;
    });
}

std::size_t NativeBinaryFileStream::copy_from_native(NativeBinaryFileStream& source, std::size_t size)
{
    if (size == 0)
        return 0;
    if (this == &source)
        throw std::system_error(std::make_error_code(std::errc::invalid_argument), "Copy file range within one stream");

#if defined(__linux__)
    auto& input = source.impl();
    auto& output = impl();
    if (!input.readable() || !output.writable() || output.append() || !input.regular_file || !output.regular_file ||
        !native_range_copy_available || !native_range_copy_same_filesystem(input.handle, output.handle))
        return 0;

    detail::throw_file_error(input.synchronize(), "Synchronize copy source");
    detail::throw_file_error(output.synchronize(), "Synchronize copy destination");

    std::size_t copied = 0;
    while (copied < size) {
        const auto result = native_copy_range(input.handle, output.handle, size - copied);
        if (!result)
            break;
        detail::throw_file_error(result->error, "Copy file range");
        if (result->bytes_transferred == 0)
            throw std::system_error(std::make_error_code(std::io_errc::stream), "Copy file range before EOF");
        copied += result->bytes_transferred;
    }
    return copied;
#else
    return 0;
#endif
}

std::error_code NativeBinaryFileStream::flush() noexcept
{
    if (!is_open())
        return std::make_error_code(std::errc::bad_file_descriptor);
    return impl().synchronize();
}

void NativeBinaryFileStream::flush(Ex)
{
    detail::throw_file_error(flush(), "Flush file stream");
}

std::size_t NativeBinaryFileStream::get_buffer_size() const noexcept
{
    return impl().buffer_capacity;
}

std::error_code NativeBinaryFileStream::set_buffer_size(std::size_t size) noexcept
{
    auto& state = impl();
    if (size == state.buffer_capacity)
        return {};
    if (std::cmp_greater(size, std::numeric_limits<std::ptrdiff_t>::max()) ||
        std::cmp_greater(size, std::numeric_limits<OffsetType>::max()))
        return std::make_error_code(std::errc::value_too_large);
    if (const auto error = state.synchronize())
        return error;

    state.cache.reset();
    state.buffer_capacity = size;
    return {};
}

void NativeBinaryFileStream::set_buffer_size(Ex, std::size_t size)
{
    detail::throw_file_error(set_buffer_size(size), "Set file stream buffer size");
}

std::error_code NativeBinaryFileStream::sync(FileSyncMode mode) noexcept
{
    auto& state = impl();
    if (!state.writable())
        return std::make_error_code(std::errc::bad_file_descriptor);
    if (mode != FileSyncMode::Full && mode != FileSyncMode::Data)
        return std::make_error_code(std::errc::invalid_argument);
    if (const auto error = state.flush_writes())
        return error;
    return native_sync(state.handle, mode);
}

void NativeBinaryFileStream::sync(Ex, FileSyncMode mode)
{
    detail::throw_file_error(sync(mode), "Sync file stream");
}

std::error_code NativeBinaryFileStream::seek(OffsetType offset, SeekFrom from) noexcept
{
    if (!is_open())
        return std::make_error_code(std::errc::bad_file_descriptor);
    if (from == SeekFrom::Begin && offset < 0)
        return std::make_error_code(std::errc::invalid_argument);

    auto& state = impl();
    if (state.state == Impl::CacheState::Reading) {
        OffsetResult base;
        switch (from) {
        case SeekFrom::Begin: base = 0; break;
        case SeekFrom::Current: base = state.position(); break;
        case SeekFrom::End: base = state.length(); break;
        default: return std::make_error_code(std::errc::invalid_argument);
        }
        if (!base)
            return base.error();
        const auto target = add_offset(*base, offset);
        if (!target)
            return target.error();
        if (*target >= state.cache_begin &&
            *target - state.cache_begin < static_cast<OffsetType>(state.cache_size)) {
            state.read_offset = static_cast<std::size_t>(*target - state.cache_begin);
            return {};
        }
        from = SeekFrom::Begin;
        offset = *target;
    } else if (const auto error = state.flush_writes()) {
        return error;
    }

    const auto result = native_seek(state.handle, offset, from);
    if (!result)
        return result.error();
    state.state = Impl::CacheState::Empty;
    return {};
}

void NativeBinaryFileStream::seek(Ex, OffsetType offset, SeekFrom from)
{
    detail::throw_file_error(seek(offset, from), "Seek file stream");
}

std::expected<OffsetType, std::error_code> NativeBinaryFileStream::tell() const noexcept
{
    if (!is_open())
        return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
    return impl().position();
}

OffsetType NativeBinaryFileStream::tell(Ex) const
{
    const auto result = tell();
    if (!result)
        throw std::system_error(result.error(), "Get file stream position");
    return *result;
}

std::expected<OffsetType, std::error_code> NativeBinaryFileStream::get_length() const noexcept
{
    if (!is_open())
        return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
    return impl().length();
}

OffsetType NativeBinaryFileStream::get_length(Ex) const
{
    const auto result = get_length();
    if (!result)
        throw std::system_error(result.error(), "Get file length");
    return *result;
}

std::error_code NativeBinaryFileStream::resize(OffsetType size) noexcept
{
    auto& state = impl();
    if (!state.writable())
        return std::make_error_code(std::errc::bad_file_descriptor);
    if (size < 0)
        return std::make_error_code(std::errc::invalid_argument);
    if (const auto error = state.synchronize())
        return error;
    return native_resize(state.handle, size);
}

void NativeBinaryFileStream::resize(Ex, OffsetType size)
{
    detail::throw_file_error(resize(size), "Resize file");
}

std::error_code NativeBinaryFileStream::reserve(OffsetType size) noexcept
{
    auto& state = impl();
    if (!state.writable())
        return std::make_error_code(std::errc::bad_file_descriptor);
    if (size < 0)
        return std::make_error_code(std::errc::invalid_argument);
    if (size == 0)
        return {};
    return native_reserve(state.handle, size);
}

void NativeBinaryFileStream::reserve(Ex, OffsetType size)
{
    detail::throw_file_error(reserve(size), "Reserve file storage");
}

std::error_code NativeBinaryFileStream::prefetch(OffsetType offset, OffsetType size) noexcept
{
    auto& state = impl();
    if (!state.readable())
        return std::make_error_code(std::errc::bad_file_descriptor);
    if (offset < 0 || size < 0)
        return std::make_error_code(std::errc::invalid_argument);
    if (const auto end = add_offset(offset, size); !end)
        return end.error();
    if (size == 0)
        return {};
    return native_prefetch(state.handle, offset, size);
}

void NativeBinaryFileStream::prefetch(Ex, OffsetType offset, OffsetType size)
{
    detail::throw_file_error(prefetch(offset, size), "Prefetch file range");
}

} // namespace clst
