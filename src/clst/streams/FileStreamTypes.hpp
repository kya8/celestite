#ifndef CLST_STREAMS_FILE_STREAM_TYPES_HPP
#define CLST_STREAMS_FILE_STREAM_TYPES_HPP

namespace clst {

enum class FileStreamMode {
    Read,
    Write,
    Append,
    ReadExtended,
    WriteExtended,
    AppendExtended
};

} // namespace clst

#endif /* CLST_STREAMS_FILE_STREAM_TYPES_HPP */
