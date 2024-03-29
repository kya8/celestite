#ifndef CLST_STREAM_BINARY_STREAM_H
#define CLST_STREAM_BINARY_STREAM_H

#include <cstdint>
#include <cstddef>
#include "clst/util/endian.h"

namespace clst::stream {

enum class SeekFrom {
    Begin,
    Current,
    End
};

enum class OpenMode {
    Read,
    Write,
    ReadWrite
};

class Base {
public:
    virtual bool isOpen() const noexcept = 0;
};

class Seekable {
public:
    using offset_type = std::int64_t;
    virtual void seek(offset_type offset, SeekFrom from = SeekFrom::Begin) = 0;
    virtual offset_type tell() const = 0;
};

class Readable {
public:
    virtual void read(void* buf, std::size_t size) = 0;
};

class Writable {
public:
    virtual void write(const void* buf, std::size_t size) = 0;
};

class BasicBinaryStreamBase : public Base, public Seekable, public Readable, public Writable {
public:
    virtual ~BasicBinaryStreamBase() = default;
};

class BinaryStreamBase : virtual public BasicBinaryStreamBase {
public:
    void copyFrom(Readable& in, std::size_t nbytes, std::size_t bufsize = 1024*1024*4);
    void patchBytes(offset_type offset, const void* buf, std::size_t n);

    // these are explicitly instantiated in source file.
    template<Endian endian, typename T, unsigned Bytes = sizeof(T)>
    T readNum();

    template<Endian endian, typename T, unsigned Bytes = sizeof(T)>
    void writeNum(const T& src);
};


} // namespace clst::stream

#endif /* CLST_STREAM_BINARY_STREAM_H */
