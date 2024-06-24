#ifndef CLST_STREAM_CURSOR_H
#define CLST_STREAM_CURSOR_H

#include "binary_stream.h"

namespace clst::stream {

class BasicMemoryCursor : virtual public BasicBinaryStreamBase {
public:
    bool open(void* buf, std::size_t size) noexcept;
    bool open(const void* buf, std::size_t size) noexcept;
    bool close() noexcept;
    bool isOpen() const noexcept override;
    bool isWritable() const noexcept;

    void read(void* buf, std::size_t size) override;
    void write(const void* buf, std::size_t size) override;
    void seek(offset_type offset, SeekFrom from = SeekFrom::Begin) override;
    offset_type tell() const override;

private:
    unsigned char* data = nullptr;
    std::size_t size = 0;
    unsigned char* cursor = nullptr;
    bool writable = false;
};

class MemoryCursor : public BasicMemoryCursor, public BinaryStreamBase {};

} // namespace clst::stream

#endif /* CLST_STREAM_CURSOR_H */
