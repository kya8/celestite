#ifndef CLST_STREAMS_FILE_STREAM_OPERATIONS_HPP
#define CLST_STREAMS_FILE_STREAM_OPERATIONS_HPP

#include "BinaryStream.hpp"
#include <ios>

namespace clst::detail {

template<typename Buffer, typename Transfer>
TransferResult transfer_file_exact(Buffer* buffer, std::size_t size, const Transfer& transfer,
                                   std::error_code no_progress_error) noexcept
{
    std::size_t done = 0;
    for (;;) {
        const auto result = transfer(buffer, size - done);
        done += result.bytes_transferred;
        if (result.error && result.error != std::errc::interrupted) {
            return {done, result.error};
        }
        if (done == size) {
            return {done, {}};
        }
        if (result.bytes_transferred == 0) {
            if (!result.error) {
                return {done, no_progress_error};
            }
        } else {
            buffer += result.bytes_transferred;
        }
    }
}

template<typename Read>
TransferResult read_file_exact(void* buffer, std::size_t size, const Read& read) noexcept
{
    return transfer_file_exact(static_cast<unsigned char*>(buffer), size, read,
        std::make_error_code(std::io_errc::stream));
}

template<typename Write>
TransferResult write_file_exact(const void* buffer, std::size_t size, const Write& write) noexcept
{
    return transfer_file_exact(static_cast<const unsigned char*>(buffer), size, write,
        std::make_error_code(std::errc::io_error));
}

} // namespace clst::detail

#endif /* CLST_STREAMS_FILE_STREAM_OPERATIONS_HPP */
