#include "lfs.h"
#include "file_stream.h"
#include "clst/common/exceptions.h"

namespace clst::stream {

struct BasicFileStream::Impl {
    FILE* fp = nullptr;
    offset_type fsize = -1;
    bool is_open = false;   // only true if fp is open, and successfully got fsize.

    Impl() noexcept = default;
    ~Impl() noexcept {
        if(fp) fclose(fp);
    }

    bool open(const char* filename, OpenMode mode) noexcept try
    {
        if (fp) return false;

        fp = fopen(filename,
        [mode]{
            if (mode == OpenMode::Read)      return "rb";
            if (mode == OpenMode::Write)     return "wb";
            if (mode == OpenMode::ReadWrite) return "r+b";
            throw error::InvalidArgument("Unhandled open mode");
        }());
        if (!fp) return false;

        if (mode == OpenMode::Read) {
            fseek64(fp, 0, SEEK_END);
            fsize = ftell64(fp);
            fseek64(fp, 0, SEEK_SET);
            if (fsize < 0) return false;
        }

        is_open = true;
        return true;
    }
    catch (error::ExceptionBase) {
        return false;
    }

    bool close() noexcept
    {
        if(!fp) return false;

        fclose(fp);
        fp = nullptr;
        is_open = false;
        fsize = -1;
        return true;
    }

    void read(void* buf, std::size_t size)
    {
        // not check is performed. Behavior on non-open stream is undefined.
        if (fread(buf, 1, size, fp) != size) {
            throw error::IoError("error reading from file");
        }
    }

    void write(const void* buf, std::size_t size)
    {
        if (fwrite(buf, 1, size, fp) != size) {
            throw error::IoError("error writing to file");
        }
    }

    void seek(offset_type offset, SeekFrom from)
    {
        if (fseek64(fp, offset,
                    [from] {
                        switch (from) {
                        case (SeekFrom::Begin)  : return SEEK_SET;
                        case (SeekFrom::Current): return SEEK_CUR;
                        case (SeekFrom::End)    : return SEEK_END;
                        }
                    }()) != 0)
        {
            throw error::IoError("error seeking file");
        }
    }

    auto tell() const
    {
        const auto pos = ftell64(fp);
        if (pos == -1) throw error::IoError("ftell error");
        return pos;
    }
};

BasicFileStream::BasicFileStream() noexcept : impl(std::make_unique<Impl>()) {}
BasicFileStream::~BasicFileStream() noexcept = default;

bool BasicFileStream::open(const char* filename, OpenMode mode) noexcept
{
    return impl->open(filename, mode);
}

bool BasicFileStream::close() noexcept
{
    return impl->close();
}

bool BasicFileStream::isOpen() const noexcept
{
    return impl->is_open;
}

std::uint64_t
BasicFileStream::getLength() const
{
    return impl->fsize;
}

void
BasicFileStream::read(void* buf, std::size_t size)
{
    impl->read(buf, size);
}

void
BasicFileStream::write(const void* buf, std::size_t size)
{
    impl->write(buf, size);
}

void
BasicFileStream::seek(offset_type offset, SeekFrom from)
{
    impl->seek(offset, from);
}

BasicFileStream::offset_type
BasicFileStream::tell() const
{
    return impl->tell();
}

} // namespace clst::stream
