#include "mp4.hpp"
#include "clst/fourcc.hpp"
#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>

namespace clst::mp4 {
namespace {

class parse_error : public error {
public:
    using error::error;
};

class unsupported_file : public error {
public:
    using error::error;
};

class not_found : public error {
public:
    using error::error;
};

class AtomErrorCategory final : public std::error_category {
public:
    const char* name() const noexcept override { return "mp4"; }

    std::string message(int value) const override
    {
        switch (static_cast<AtomError>(value)) {
        case AtomError::invalid_size:      return "Invalid atom size.";
        case AtomError::beyond_eof:        return "Atom goes beyond EOF.";
        case AtomError::unsupported_type:  return "Extended type is not supported.";
        case AtomError::not_found:         return "Could not find atom in range";
        }
        return "Unknown MP4 atom error";
    }
};

const AtomErrorCategory atom_errors;

Mp4Stream::AtomInfo atom_value(std::expected<Mp4Stream::AtomInfo, std::error_code> result)
{
    if (result) {
        return *result;
    }

    const auto error = result.error();
    if (error.category() == atom_errors) {
        switch (static_cast<AtomError>(error.value())) {
        case AtomError::invalid_size:
        case AtomError::beyond_eof:
            throw parse_error(error.message());
        case AtomError::unsupported_type:
            throw unsupported_file(error.message());
        case AtomError::not_found:
            throw not_found(error.message());
        }
    }
    throw std::system_error(error, "Read MP4 atom");
}

bool valid_atom_range(const Mp4Stream::AtomInfo& atom) noexcept
{
    constexpr auto max_offset = static_cast<uint64_t>(std::numeric_limits<OffsetType>::max());
    return atom.size >= atom.header_size && atom.size <= max_offset && atom.offset <= max_offset - atom.size;
}

} // namespace

std::error_code make_error_code(AtomError reason) noexcept
{
    return {static_cast<int>(reason), atom_errors};
}

bool Mp4Stream::open(const char* filename) noexcept
{
    return !BinaryFileStream::open(filename, FileStreamMode::Read);
}

void Mp4Stream::open(Ex, const char* filename)
{
    BinaryFileStream::open(ex, filename, FileStreamMode::Read);
}

std::expected<Mp4Stream::AtomInfo, std::error_code> Mp4Stream::parse_atom() noexcept
{
    const auto offset = tell();
    if (!offset) {
        return std::unexpected(offset.error());
    }

    AtomInfo atom{.offset = static_cast<uint64_t>(*offset), .size = 0, .fourcc = 0, .header_size = 8};
    uint32_t size32;
    if (const auto error = read_nums(size32, atom.fourcc)) {
        return std::unexpected(error);
    }
    if (atom.fourcc == "uuid"_fc) {
        return std::unexpected(make_error_code(AtomError::unsupported_type));
    }

    atom.size = size32;
    if (size32 == 1) {
        if (const auto error = read_num(atom.size)) {
            return std::unexpected(error);
        }
        atom.header_size = 16;
    }

    const auto length = get_length();
    if (!length) {
        return std::unexpected(length.error());
    }
    if (*offset > *length) {
        return std::unexpected(make_error_code(AtomError::beyond_eof));
    }
    const auto remaining = static_cast<uint64_t>(*length - *offset);
    if (size32 == 0) {
        atom.size = remaining;
    }

    if (atom.size < atom.header_size) {
        return std::unexpected(make_error_code(AtomError::invalid_size));
    }
    if (atom.size > remaining) {
        return std::unexpected(make_error_code(AtomError::beyond_eof));
    }
    return atom;
}

Mp4Stream::AtomInfo Mp4Stream::parse_atom(Ex)
{
    return atom_value(parse_atom());
}

std::expected<Mp4Stream::AtomInfo, std::error_code>
Mp4Stream::seek_to_atom_data(uint32_t fourcc, uint64_t max_range) noexcept
{
    const auto start = tell();
    if (!start) {
        return std::unexpected(start.error());
    }

    for (;;) {
        const auto atom = parse_atom();
        if (!atom) {
            return std::unexpected(atom.error());
        }
        const auto end = static_cast<OffsetType>(atom->end_offset());
        const auto span = static_cast<uint64_t>(end - *start);
        if (span > max_range) {
            break;
        }
        if (atom->fourcc == fourcc) {
            return atom;
        }
        if (const auto error = seek(end)) {
            return std::unexpected(error);
        }
        const auto length = get_length();
        if (!length) {
            return std::unexpected(length.error());
        }
        if (end == *length || span == max_range) {
            break;
        }
    }
    return std::unexpected(make_error_code(AtomError::not_found));
}

Mp4Stream::AtomInfo Mp4Stream::seek_to_atom_data(Ex, uint32_t fourcc, uint64_t max_range)
{
    return atom_value(seek_to_atom_data(fourcc, max_range));
}

std::expected<Mp4Stream::AtomInfo, std::error_code>
Mp4Stream::seek_to_atom_data(uint32_t fourcc, const AtomInfo& container) noexcept
{
    if (!valid_atom_range(container)) {
        return std::unexpected(make_error_code(AtomError::invalid_size));
    }
    if (const auto error = seek(static_cast<OffsetType>(container.data_offset()))) {
        return std::unexpected(error);
    }
    return seek_to_atom_data(fourcc, container.data_size());
}

Mp4Stream::AtomInfo Mp4Stream::seek_to_atom_data(Ex, uint32_t fourcc, const AtomInfo& container)
{
    return atom_value(seek_to_atom_data(fourcc, container));
}

std::expected<std::vector<Mp4Stream::AtomInfo>, std::error_code> Mp4Stream::get_all_atoms(uint64_t max_range) noexcept try
{
    const auto start = tell();
    if (!start) {
        return std::unexpected(start.error());
    }

    std::vector<AtomInfo> atoms;
    for (;;) {
        const auto atom = parse_atom();
        if (!atom) {
            break;
        }
        const auto end = static_cast<OffsetType>(atom->end_offset());
        const auto span = static_cast<uint64_t>(end - *start);
        if (span > max_range) {
            break;
        }
        atoms.push_back(*atom);
        if (seek(end) || span == max_range) {
            break;
        }
    }
    return atoms;
} catch (const std::bad_alloc&) {
    return std::unexpected(std::make_error_code(std::errc::not_enough_memory));
} catch (const std::length_error&) {
    return std::unexpected(std::make_error_code(std::errc::value_too_large));
}

std::vector<Mp4Stream::AtomInfo> Mp4Stream::get_all_atoms(Ex, uint64_t max_range)
{
    return detail::stream_value(get_all_atoms(max_range), "List MP4 atoms");
}

std::expected<std::vector<unsigned char>, std::error_code> Mp4Stream::read_atom_data(const AtomInfo& atom) noexcept try
{
    if (!valid_atom_range(atom)) {
        return std::unexpected(make_error_code(AtomError::invalid_size));
    }
    if (const auto error = seek(static_cast<OffsetType>(atom.data_offset()))) {
        return std::unexpected(error);
    }

    std::vector<unsigned char> data;
    if (atom.data_size() > data.max_size()) {
        return std::unexpected(std::make_error_code(std::errc::value_too_large));
    }
    data.resize(static_cast<std::size_t>(atom.data_size()));
    if (const auto read = read_exact(data.data(), data.size()); read.error) {
        return std::unexpected(read.error);
    }
    return data;
} catch (const std::bad_alloc&) {
    return std::unexpected(std::make_error_code(std::errc::not_enough_memory));
}

std::vector<unsigned char> Mp4Stream::read_atom_data(Ex, const AtomInfo& atom)
{
    return detail::stream_value(read_atom_data(atom), "Read MP4 atom data");
}

bool Mp4Stream::verify() noexcept
{
    if (seek(0)) {
        return false;
    }
    const auto length = get_length();
    if (!length) {
        return false;
    }
    const auto atoms = get_all_atoms(*length);
    const auto rewind_error = seek(0);
    return atoms && !rewind_error && std::ranges::count(*atoms, "moov"_fc, &AtomInfo::fourcc) == 1;
}

} // namespace clst::mp4
