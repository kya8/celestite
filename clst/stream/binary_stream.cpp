#include "binary_stream.h"
#include <memory>
#include "clst/util/endian_rw.h"
#include "clst/common/exceptions.h"
#include "clst/util/types/ints.h"

namespace clst::stream
{

void
BinaryStreamBase::copyFrom(Readable& in, std::size_t nbytes, std::size_t bufsize)
{
    if (bufsize == 0) {
        throw error::InvalidArgument("buffer size must be positive");
    }

    const auto buf = std::make_unique<unsigned char[]>(bufsize);
    while (nbytes > 0) {
        const auto n = nbytes >= bufsize? bufsize : nbytes;
        in.read(buf.get(), n);
        write(buf.get(), n);
        nbytes -= n;
    }
}

void
BinaryStreamBase::patchBytes(offset_type offset, const void* buf, std::size_t n)
{
    const auto mark = tell();
    seek(offset, SeekFrom::Begin);
    write(buf, n);
    seek(mark, SeekFrom::Begin);
}

template<Endian endian, typename T, unsigned Bytes>
T
BinaryStreamBase::readNum()
{
    unsigned char buf[Bytes];
    read(buf, Bytes);
    if constexpr (endian == Endian::BE) {
        return util::readBE<T, Bytes>(buf);
    }
    else if constexpr (endian == Endian::LE) {
        return util::readLE<T, Bytes>(buf);
    }
    else {
        static_assert(false, "endian must be LE or BE");
    }
}

template<Endian endian, typename T, unsigned Bytes>
void
BinaryStreamBase::writeNum(const T& src)
{
    unsigned char buf[Bytes];
    if constexpr (endian == Endian::BE) {
        util::writeBE<T, Bytes>(src, buf);
    }
    else if constexpr (endian == Endian::LE) {
        util::writeLE<T, Bytes>(src, buf);
    }
    else {
        static_assert(false, "endian must be LE or BE");
    }
    write(buf, Bytes);
}


/* explicit instantiations */
#define MAKE_INSTANTIATION_READ(ENDIAN, T, BYTES)   template T BinaryStreamBase::readNum<Endian::ENDIAN, T, BYTES>()
#define MAKE_INSTANTIATION_READ_DEFAULT(ENDIAN, T)  template T BinaryStreamBase::readNum<Endian::ENDIAN, T>()
#define MAKE_INSTANTIATION_WRITE(ENDIAN, T, BYTES)  template void BinaryStreamBase::writeNum<Endian::ENDIAN, T, BYTES>(const T&)
#define MAKE_INSTANTIATION_WRITE_DEFAULT(ENDIAN, T) template void BinaryStreamBase::writeNum<Endian::ENDIAN, T>(const T&)

MAKE_INSTANTIATION_READ_DEFAULT(LE, char);
MAKE_INSTANTIATION_READ_DEFAULT(BE, char);
MAKE_INSTANTIATION_READ_DEFAULT(LE, unsigned char);
MAKE_INSTANTIATION_READ_DEFAULT(BE, unsigned char);
MAKE_INSTANTIATION_READ_DEFAULT(LE, signed char);
MAKE_INSTANTIATION_READ_DEFAULT(BE, signed char);
MAKE_INSTANTIATION_READ_DEFAULT(LE, short);
MAKE_INSTANTIATION_READ_DEFAULT(BE, short);
MAKE_INSTANTIATION_READ_DEFAULT(LE, unsigned short);
MAKE_INSTANTIATION_READ_DEFAULT(BE, unsigned short);
MAKE_INSTANTIATION_READ_DEFAULT(LE, int);
MAKE_INSTANTIATION_READ_DEFAULT(BE, int);
MAKE_INSTANTIATION_READ_DEFAULT(LE, unsigned int);
MAKE_INSTANTIATION_READ_DEFAULT(BE, unsigned int);
MAKE_INSTANTIATION_READ_DEFAULT(LE, long);
MAKE_INSTANTIATION_READ_DEFAULT(BE, long);
MAKE_INSTANTIATION_READ_DEFAULT(LE, unsigned long);
MAKE_INSTANTIATION_READ_DEFAULT(BE, unsigned long);
MAKE_INSTANTIATION_READ_DEFAULT(LE, long long);
MAKE_INSTANTIATION_READ_DEFAULT(BE, long long);
MAKE_INSTANTIATION_READ_DEFAULT(LE, unsigned long long);
MAKE_INSTANTIATION_READ_DEFAULT(BE, unsigned long long);
MAKE_INSTANTIATION_READ_DEFAULT(LE, float);
MAKE_INSTANTIATION_READ_DEFAULT(BE, float);
MAKE_INSTANTIATION_READ_DEFAULT(LE, double);
MAKE_INSTANTIATION_READ_DEFAULT(BE, double);
MAKE_INSTANTIATION_READ(LE, types::ints::u32, 3);
MAKE_INSTANTIATION_READ(BE, types::ints::u32, 3);

MAKE_INSTANTIATION_WRITE_DEFAULT(LE, char);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, char);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, unsigned char);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, unsigned char);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, signed char);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, signed char);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, short);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, short);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, unsigned short);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, unsigned short);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, int);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, int);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, unsigned int);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, unsigned int);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, long);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, long);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, unsigned long);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, unsigned long);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, long long);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, long long);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, unsigned long long);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, unsigned long long);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, float);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, float);
MAKE_INSTANTIATION_WRITE_DEFAULT(LE, double);
MAKE_INSTANTIATION_WRITE_DEFAULT(BE, double);


} // namespace clst::stream
