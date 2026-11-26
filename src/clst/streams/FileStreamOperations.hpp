#ifndef CLST_STREAMS_FILE_STREAM_OPERATIONS_HPP
#define CLST_STREAMS_FILE_STREAM_OPERATIONS_HPP

#include <cstddef>
#include <ios>
#include <system_error>

namespace clst::detail {

inline void throw_file_error(std::error_code error, const char* operation)
{
    if (error) {
        throw std::system_error(error, operation);
    }
}

template<typename Buffer, typename Transfer>
std::size_t transfer_file_exact(Buffer* buffer, std::size_t size, Transfer transfer,
                                std::error_code no_progress_error, const char* operation)
{
    auto remaining = size;
    for (;;) {
        const auto result = transfer(buffer, remaining);
        throw_file_error(result.error, operation);
        if (result.bytes_transferred == remaining) {
            return size;
        }
        if (result.bytes_transferred == 0) {
            throw std::system_error(no_progress_error, operation);
        }
        buffer += result.bytes_transferred;
        remaining -= result.bytes_transferred;
    }
}

template<typename Read>
std::size_t read_file_exact(void* buffer, std::size_t size, Read read)
{
    return transfer_file_exact(static_cast<unsigned char*>(buffer), size, read,
        std::make_error_code(std::io_errc::stream), "Reading file");
}

template<typename Write>
std::size_t write_file_exact(const void* buffer, std::size_t size, Write write)
{
    return transfer_file_exact(static_cast<const unsigned char*>(buffer), size, write,
        std::make_error_code(std::errc::io_error), "Writing file");
}

} // namespace clst::detail

#endif /* CLST_STREAMS_FILE_STREAM_OPERATIONS_HPP */
