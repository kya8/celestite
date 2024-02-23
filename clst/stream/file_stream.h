#ifndef CLST_STREAM_FILE_STREAM_H
#define CLST_STREAM_FILE_STREAM_H

#include "binary_stream.h"
#include <memory>

namespace clst::stream {

class BasicFileStream : virtual public BasicBinaryStreamBase {
public:
    BasicFileStream() noexcept;
    ~BasicFileStream() noexcept override;

    bool open(const char* filename, const char* mode) noexcept;
    bool close() noexcept;
    bool is_open() const noexcept override;

    std::uint64_t get_length() const;
    void read(void* buf, std::size_t size) override;
    void write(const void* buf, std::size_t size) override;
    void seek(offset_type offset, SeekFrom from = SeekFrom::Begin) override;
    offset_type tell() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


class FileStream : public BasicFileStream, public BinaryStreamBase {};

} // namespace clst::stream

#endif /* CLST_STREAM_FILE_STREAM_H */
