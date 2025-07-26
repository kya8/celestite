#ifndef CLST_BINARY_FILE_STREAM_HPP
#define CLST_BINARY_FILE_STREAM_HPP

#include "clst/binary_stream.hpp"
#include "clst/detail/lfs.h"  // 64-bit ftell/fseek
#include <string>
#include <cassert>
#include "clst/builtins.h"

namespace clst {

enum class FileStreamMode {
    Read,
    Write,
    Append,
    ReadExtended,
    WriteExtended,
    AppendExtended
};

class BinaryFileStream : public RWStream<BinaryFileStream> {
public:
    BinaryFileStream() noexcept = default;
    ~BinaryFileStream() noexcept;

    BinaryFileStream(const BinaryFileStream&) = delete;
    BinaryFileStream& operator=(const BinaryFileStream&) = delete;
    BinaryFileStream(BinaryFileStream&& rhs) noexcept;
    BinaryFileStream& operator=(BinaryFileStream&& rhs) noexcept;

    bool open(const char* filename, FileStreamMode mode = FileStreamMode::Read) noexcept;
    bool close() noexcept;
    bool is_open() const noexcept;
    OffsetType get_length() const noexcept;

    void read(void* buf, std::size_t n)
    {
        assert(is_open());
        if (n == 0)
            return;
        if (fread(buf, n, 1, fp) != 1)
            throw StreamIoError("File stream read error");
    }
    void write(const void* buf, std::size_t n)
    {
        assert(is_open());
        if (n == 0)
            return;
        if (fwrite(buf, n, 1, fp) != 1)
            throw StreamIoError("File stream write error");
    }
    void seek(OffsetType offset, SeekFrom from = SeekFrom::Begin)
    {
        assert(is_open());
        if (detail::fseek64(fp, offset, [from]{
            switch(from) {
                case(SeekFrom::Begin)  : return SEEK_SET;
                case(SeekFrom::Current): return SEEK_CUR;
                case(SeekFrom::End)    : return SEEK_END;
                default: assert(false && "Bad SeekFrom"); CLST_UNREACHABLE;
            }
        }()) != 0)
            throw StreamIoError("File stream seek error");
    }
    OffsetType tell() const
    {
        assert(is_open());
        const auto ret = detail::ftell64(fp);
        if (ret == -1L)
            throw StreamIoError{"File stream tell error"};
        return ret;
    }

private:
    FILE* fp = nullptr;
    OffsetType fsize = -1;
};

} // namespace clst

#endif // CLST_BINARY_FILE_STREAM_HPP
