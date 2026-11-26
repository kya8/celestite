#ifndef CLST_STREAMS_FILE_STREAM_TYPES_HPP
#define CLST_STREAMS_FILE_STREAM_TYPES_HPP

#include <cstddef>
#include <system_error>

namespace clst {

enum class FileStreamMode {
    Read,
    Write,
    Append,
    ReadExtended,
    WriteExtended,
    AppendExtended
};

// A transfer can make progress and encounter an error in the same call.
struct TransferResult {
    std::size_t bytes_transferred = 0;
    std::error_code error;
};

} // namespace clst

#endif /* CLST_STREAMS_FILE_STREAM_TYPES_HPP */
