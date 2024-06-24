#include "cursor.h"
#include <cstring>
#include <clst/common/exceptions.h>

namespace clst::stream {

bool
BasicMemoryCursor::open(void* buf, std::size_t size) noexcept
{
    data = static_cast<unsigned char*>(buf);
    cursor = data;
    this->size = size;
    writable = true;
    return data;
}

bool
BasicMemoryCursor::open(const void* buf, std::size_t size) noexcept
{
    data = static_cast<unsigned char*>(const_cast<void*>(buf));
    cursor = data;
    this->size = size;
    writable = false;
    return data;
}

bool
BasicMemoryCursor::close() noexcept
{
    data = nullptr;
    writable = false;
    return true;
}

bool
BasicMemoryCursor::isOpen() const noexcept
{
    return data;
}

bool
BasicMemoryCursor::isWritable() const noexcept
{
    return writable;
}

void
BasicMemoryCursor::read(void* buf, std::size_t n)
{
    if (n > size - (cursor - data)) throw error::IoError{"Cursor read out-of-bound"};
    std::memcpy(buf, cursor, n);
    cursor += n;
}

void
BasicMemoryCursor::write(const void* buf, std::size_t n)
{
    if (!writable) throw error::IoError{"Cursor is not writable"};
    if (n > size - (cursor - data)) throw error::IoError{"Cursor write out-of-bound"};
    std::memcpy(cursor, buf, n);
    cursor += n;
}

void
BasicMemoryCursor::seek(offset_type offset, SeekFrom from)
{
    const auto ssize = static_cast<offset_type>(size);
    switch (from) {
    case SeekFrom::Begin:
        if (offset > ssize || offset < 0) throw error::IoError{"Cursor seek out-of-range"};
        cursor = data + offset;
        break;
    case SeekFrom::Current: {
        const auto pos = cursor - data + offset;
        if (pos > ssize || pos < 0) throw error::IoError{"Cursor seek out-of-range"};
        cursor += offset;
        break;
    }
    case SeekFrom::End:
        if (offset < -ssize || offset > 0) throw error::IoError{"Cursor seek out-of-range"};
        cursor = data + ssize + offset;
        break;
    }
}

BasicMemoryCursor::offset_type
BasicMemoryCursor::tell() const
{
    return cursor - data;
}

} // namespace clst::stream
