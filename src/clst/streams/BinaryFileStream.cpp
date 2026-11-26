#include "lfs.hpp" // Define large-file feature macros before system headers.

#include "BinaryFileStream.hpp"
#include "FileStreamOperations.hpp"

#include <cerrno>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <utility>

namespace clst {
namespace {

const char* mode_string(FileStreamMode mode) noexcept
{
    switch (mode) {
    case FileStreamMode::Read:           return "rb";
    case FileStreamMode::Write:          return "wb";
    case FileStreamMode::Append:         return "ab";
    case FileStreamMode::ReadExtended:   return "r+b";
    case FileStreamMode::WriteExtended:  return "w+b";
    case FileStreamMode::AppendExtended: return "a+b";
    }
    return nullptr;
}

int seek_origin(SeekFrom from) noexcept
{
    switch (from) {
    case SeekFrom::Begin:   return SEEK_SET;
    case SeekFrom::Current: return SEEK_CUR;
    case SeekFrom::End:     return SEEK_END;
    }
    return -1;
}

std::error_code file_error() noexcept
{
    return errno != 0 ? std::error_code(errno, std::generic_category()) :
                       std::make_error_code(std::errc::io_error);
}

std::error_code flush_output(std::FILE* fp) noexcept
{
    errno = 0;
    if (std::fflush(fp) != 0) {
        return file_error();
    }
    return {};
}

} // namespace

BinaryFileStream::BinaryFileStream(const char* filename, FileStreamMode mode)
{
    open_ex(filename, mode);
}

BinaryFileStream::~BinaryFileStream() noexcept
{
    static_cast<void>(close());
}

BinaryFileStream::BinaryFileStream(BinaryFileStream&& rhs) noexcept
    : fp_(std::exchange(rhs.fp_, nullptr)), mode_(rhs.mode_),
      direction_(std::exchange(rhs.direction_, Direction::None))
{
}

BinaryFileStream& BinaryFileStream::operator=(BinaryFileStream&& rhs) noexcept
{
    if (this != &rhs) {
        static_cast<void>(close());
        fp_ = std::exchange(rhs.fp_, nullptr);
        mode_ = rhs.mode_;
        direction_ = std::exchange(rhs.direction_, Direction::None);
    }
    return *this;
}

std::error_code BinaryFileStream::open(const char* filename, FileStreamMode mode) noexcept
{
    if (is_open()) {
        return std::make_error_code(std::errc::device_or_resource_busy);
    }
    const auto* flags = mode_string(mode);
    if (filename == nullptr || flags == nullptr) {
        return std::make_error_code(std::errc::invalid_argument);
    }

#ifdef _MSC_VER
    if (const auto error = ::fopen_s(&fp_, filename, flags); error != 0) {
        return {error, std::generic_category()};
    }
#else
    errno = 0;
    fp_ = std::fopen(filename, flags);
    if (fp_ == nullptr) {
        return file_error();
    }
#endif

    mode_ = mode;
    return {};
}

std::error_code BinaryFileStream::close() noexcept
{
    auto* fp = std::exchange(fp_, nullptr);
    direction_ = Direction::None;
    if (fp != nullptr) {
        errno = 0;
        if (std::fclose(fp) != 0) {
            return file_error();
        }
    }
    return {};
}

std::error_code BinaryFileStream::flush() noexcept
{
    if (!is_open()) {
        return std::make_error_code(std::errc::bad_file_descriptor);
    }
    if (direction_ == Direction::Write) {
        return flush_output(fp_);
    }
    return {};
}

bool BinaryFileStream::is_open() const noexcept
{
    return fp_ != nullptr;
}

TransferResult BinaryFileStream::read(void* buf, std::size_t n) noexcept
{
    if (!is_open() || mode_ == FileStreamMode::Write || mode_ == FileStreamMode::Append) {
        return {.error = std::make_error_code(std::errc::bad_file_descriptor)};
    }
    if (n == 0) {
        return {};
    }
    if (buf == nullptr) {
        return {.error = std::make_error_code(std::errc::invalid_argument)};
    }

    if (const auto error = flush(); error) {
        return {.error = error};
    }
    direction_ = Direction::Read;

    std::clearerr(fp_);
    errno = 0;
    const auto count = std::fread(buf, 1, n, fp_);
    const auto error = file_error();
    return {count, std::ferror(fp_) ? error : std::error_code{}};
}

TransferResult BinaryFileStream::write(const void* buf, std::size_t n) noexcept
{
    if (!is_open() || mode_ == FileStreamMode::Read) {
        return {.error = std::make_error_code(std::errc::bad_file_descriptor)};
    }
    if (n == 0) {
        return {};
    }
    if (buf == nullptr) {
        return {.error = std::make_error_code(std::errc::invalid_argument)};
    }

    if (direction_ == Direction::Read) {
        if (const auto error = seek(0, SeekFrom::Current); error) {
            return {.error = error};
        }
    }
    direction_ = Direction::Write;

    std::clearerr(fp_);
    errno = 0;
    const auto count = std::fwrite(buf, 1, n, fp_);
    const auto error = file_error();
    return {count, std::ferror(fp_) ? error : std::error_code{}};
}

std::error_code BinaryFileStream::seek(OffsetType offset, SeekFrom from) noexcept
{
    if (!is_open()) {
        return std::make_error_code(std::errc::bad_file_descriptor);
    }
    const auto origin = seek_origin(from);
    if (origin < 0) {
        return std::make_error_code(std::errc::invalid_argument);
    }

    errno = 0;
    if (detail::fseek64(fp_, offset, origin) != 0) {
        return file_error();
    }
    direction_ = Direction::None;
    return {};
}

std::expected<OffsetType, std::error_code> BinaryFileStream::tell() const noexcept
{
    if (!is_open()) {
        return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
    }
    errno = 0;
    const auto position = detail::ftell64(fp_);
    if (position < 0) {
        return std::unexpected(file_error());
    }
    return position;
}

std::expected<OffsetType, std::error_code> BinaryFileStream::get_length() const noexcept
{
    if (!is_open()) {
        return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
    }
    if (direction_ == Direction::Write) {
        if (const auto error = flush_output(fp_); error) {
            return std::unexpected(error);
        }
    }

    errno = 0;
#ifdef _WIN32
    struct _stat64 info;
    const auto result = ::_fstat64(::_fileno(fp_), &info);
#else
    struct stat info;
    const auto result = ::fstat(::fileno(fp_), &info);
#endif
    if (result != 0) {
        return std::unexpected(file_error());
    }
    return info.st_size;
}

void BinaryFileStream::open_ex(const char* filename, FileStreamMode mode)
{
    detail::throw_file_error(open(filename, mode), "Opening file");
}

void BinaryFileStream::close_ex()
{
    detail::throw_file_error(close(), "Closing file");
}

void BinaryFileStream::flush_ex()
{
    detail::throw_file_error(flush(), "Flushing file");
}

std::size_t BinaryFileStream::read_ex(void* buf, std::size_t n)
{
    return detail::read_file_exact(buf, n, [this](void* data, std::size_t size) { return read(data, size); });
}

std::size_t BinaryFileStream::write_ex(const void* buf, std::size_t n)
{
    return detail::write_file_exact(buf, n, [this](const void* data, std::size_t size) { return write(data, size); });
}

void BinaryFileStream::seek_ex(OffsetType offset, SeekFrom from)
{
    detail::throw_file_error(seek(offset, from), "Seeking file");
}

OffsetType BinaryFileStream::tell_ex() const
{
    const auto position = tell();
    if (!position) {
        throw std::system_error(position.error(), "Getting file position");
    }
    return *position;
}

OffsetType BinaryFileStream::get_length_ex() const
{
    const auto length = get_length();
    if (!length) {
        throw std::system_error(length.error(), "Getting file length");
    }
    return *length;
}

} // namespace clst
