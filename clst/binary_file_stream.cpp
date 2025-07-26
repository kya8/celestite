#include "clst/binary_file_stream.hpp"

namespace clst {

BinaryFileStream::~BinaryFileStream() noexcept
{
    if (fp_) {
        fclose(fp_);
    }
}

BinaryFileStream::BinaryFileStream(BinaryFileStream&& rhs) noexcept : fp_(rhs.fp_), file_size_(rhs.file_size_)
{
    rhs.fp_ = nullptr;
}

BinaryFileStream& BinaryFileStream::operator=(BinaryFileStream&& rhs) noexcept
{
    close();
    fp_ = rhs.fp_;
    file_size_ = rhs.file_size_;
    rhs.fp_ = nullptr;
    return *this;
}

bool
BinaryFileStream::open(const char* filename, FileStreamMode mode) noexcept
{
    if (fp_) {
        return false;
    }

    const auto mode_str = [mode]() -> const char* {
        switch (mode) {
        case FileStreamMode::Read:
            return "rb";
        case FileStreamMode::Write:
            return "wb";
        case FileStreamMode::Append:
            return "ab";
        case FileStreamMode::ReadExtended:
            return "r+b";
        case FileStreamMode::WriteExtended:
            return "w+b";
        case FileStreamMode::AppendExtended:
            return "a+b";
        }
        return nullptr;
    }();
    if (!mode_str) {
        return false;
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
    fp_ = fopen(filename, mode_str);
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    if (!fp_) {
        return false;
    }

    if (mode == FileStreamMode::Read || mode == FileStreamMode::ReadExtended) {
        if (detail::fseek64(fp_, 0, SEEK_END) != 0) {
            return false;
        }
        file_size_ = detail::ftell64(fp_);
        detail::fseek64(fp_, 0, SEEK_SET);
        if (file_size_ < 0) {
            return false;
        }
    }
    return true;
}

bool BinaryFileStream::close() noexcept
{
    if (!fp_) {
        return false;
    }

    fclose(fp_);
    fp_ = nullptr;
    file_size_ = -1;
    return true;
}

bool BinaryFileStream::is_open() const noexcept
{
    return fp_ != nullptr;
}

OffsetType BinaryFileStream::get_length() const noexcept
{
    return file_size_;
}

} // namespace clst
