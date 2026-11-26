#ifndef CLST_MP4_MP4_HPP
#define CLST_MP4_MP4_HPP

#include "clst/streams/BinaryFileStream.hpp"
#include <expected>
#include <system_error>
#include <vector>

namespace clst::mp4 {

// base exception class
class error : public StreamError {
public:
    using StreamError::StreamError;
};

enum class AtomError { invalid_size = 1, beyond_eof, unsupported_type, not_found };
std::error_code make_error_code(AtomError reason) noexcept;

// Binary stream for MP4 file.
// This class is mainly for mp4_merge.
class Mp4Stream : public BinaryFileStream {
public:
    using BinaryFileStream::BinaryFileStream;

    [[nodiscard]] bool open(const char* filename) noexcept;
    void open(Ex, const char* filename);

    // This holds stream-specific info for an atom.
    struct AtomInfo {
        uint64_t offset;       // offset to header within file
        uint64_t size;         // size reported in header
        uint32_t fourcc;       // atom type
        uint32_t header_size;  // size of header, either 8 or 16

        uint64_t data_size()   const noexcept { return size - header_size; }
        uint64_t data_offset() const noexcept { return offset + header_size; }
        uint64_t end_offset()  const noexcept { return offset + size; }
    };

    // Must be used at the beginning of an atom header.
    [[nodiscard]] std::expected<AtomInfo, std::error_code> parse_atom() noexcept;
    [[nodiscard]] AtomInfo parse_atom(Ex);

    // Must be used at the start of a container atom, or beginning of file.
    [[nodiscard]] std::expected<AtomInfo, std::error_code> seek_to_atom_data(uint32_t fourcc, uint64_t max_range) noexcept;
    AtomInfo seek_to_atom_data(Ex, uint32_t fourcc, uint64_t max_range);

    // Additional overload that seeks inside a container box.
    // Will seek to container data first.
    [[nodiscard]] std::expected<AtomInfo, std::error_code> seek_to_atom_data(uint32_t fourcc, const AtomInfo& container) noexcept;
    AtomInfo seek_to_atom_data(Ex, uint32_t fourcc, const AtomInfo& container);

    // Get the valid prefix of atoms within a range.
    // Should be used at the start of a container atom, or beginning of file.
    [[nodiscard]] std::expected<std::vector<AtomInfo>, std::error_code> get_all_atoms(uint64_t max_range) noexcept;
    [[nodiscard]] std::vector<AtomInfo> get_all_atoms(Ex, uint64_t max_range);

    // Simple check for mp4 file format.
    // This will loop over all top-level atoms, and look for the 'moov' box.
    [[nodiscard]] bool verify() noexcept;

    [[nodiscard]] std::expected<std::vector<unsigned char>, std::error_code> read_atom_data(const AtomInfo& atom) noexcept;
    [[nodiscard]] std::vector<unsigned char> read_atom_data(Ex, const AtomInfo& atom);

};

} // namespace clst::mp4


#endif /* CLST_MP4_MP4_HPP */
