#include "test.h"
#include "clst/mp4/mp4.hpp"
#include "clst/streams/BinaryFileStream.hpp"
#include "clst/streams/FileStreamOperations.hpp"
#include "clst/streams/NativeBinaryFileStream.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <ios>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

using clst::BinaryFileStream;
using clst::FileAccessPattern;
using clst::FileStreamMode;
using clst::FileSyncMode;
using clst::NativeBinaryFileStream;
using clst::OffsetType;
using clst::SeekFrom;
using clst::adopt_handle;

static_assert(clst::ReadStream<BinaryFileStream> && clst::WriteStream<BinaryFileStream>);
static_assert(clst::ReadStream<NativeBinaryFileStream> && clst::WriteStream<NativeBinaryFileStream>);
static_assert(std::is_nothrow_move_constructible_v<NativeBinaryFileStream>);
static_assert(std::is_nothrow_move_assignable_v<NativeBinaryFileStream>);
static_assert(!std::is_copy_constructible_v<NativeBinaryFileStream>);
static_assert(!std::is_copy_assignable_v<NativeBinaryFileStream>);
#ifdef _WIN32
static_assert(std::is_same_v<decltype(std::declval<NativeBinaryFileStream&>().get_handle()), void*>);
#else
static_assert(std::is_same_v<decltype(std::declval<NativeBinaryFileStream&>().get_handle()), int>);
#endif

class TempDirectory {
public:
    TempDirectory()
    {
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto root = std::filesystem::temp_directory_path();
        for (int attempt = 0; attempt < 10; ++attempt) {
            path = root / ("clst-stream-test-" + std::to_string(ticks) + "-" + std::to_string(attempt));
            if (std::filesystem::create_directory(path)) {
                return;
            }
        }
        CLST_ASSERT(false);
    }

    ~TempDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }

    [[nodiscard]] std::string filename(const char* name) const { return (path / name).string(); }

private:
    std::filesystem::path path;
};

OffsetType native_length(const NativeBinaryFileStream& stream)
{
#ifdef _WIN32
    LARGE_INTEGER length;
    CLST_ASSERT(GetFileSizeEx(stream.get_handle(), &length));
    return length.QuadPart;
#else
    struct stat info;
    CLST_ASSERT(fstat(stream.get_handle(), &info) == 0);
    return info.st_size;
#endif
}

OffsetType native_position(const NativeBinaryFileStream& stream)
{
#ifdef _WIN32
    LARGE_INTEGER position;
    CLST_ASSERT(SetFilePointerEx(stream.get_handle(), LARGE_INTEGER{}, &position, FILE_CURRENT));
    return position.QuadPart;
#else
    return lseek(stream.get_handle(), 0, SEEK_CUR);
#endif
}

NativeBinaryFileStream::NativeHandle open_native_handle(const std::string& filename, bool writable)
{
#ifdef _WIN32
    const auto access = writable ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;
    const auto handle = CreateFileA(filename.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    CLST_ASSERT(handle != INVALID_HANDLE_VALUE);
#else
    const auto handle = ::open(filename.c_str(), writable ? O_RDWR : O_RDONLY);
    CLST_ASSERT(handle != -1);
#endif
    return handle;
}

bool native_handle_is_open(NativeBinaryFileStream::NativeHandle handle)
{
#ifdef _WIN32
    DWORD flags = 0;
    return GetHandleInformation(handle, &flags) != 0;
#else
    return ::fcntl(handle, F_GETFD) != -1;
#endif
}

void check_adoption(const std::string& filename)
{
    NativeBinaryFileStream writer(filename.c_str(), FileStreamMode::Write);
    writer.write_ex("abcdef", 6);
    writer.close_ex();

    const auto handle = open_native_handle(filename, true);
#ifdef _WIN32
    LARGE_INTEGER offset{};
    offset.QuadPart = 2;
    CLST_ASSERT(SetFilePointerEx(handle, offset, nullptr, FILE_BEGIN));
#else
    CLST_ASSERT(::lseek(handle, 2, SEEK_SET) == 2);
#endif
    NativeBinaryFileStream stream(adopt_handle, handle, FileStreamMode::ReadExtended);
    CLST_ASSERT(stream.get_handle() == handle && stream.tell_ex() == 2);
    std::array<char, 2> data{};
    CLST_ASSERT(stream.read_ex(data.data(), data.size()) == data.size());
    CLST_ASSERT(std::string_view(data.data(), data.size()) == "cd");

    const auto incoming = open_native_handle(filename, false);
    CLST_ASSERT(stream.adopt(incoming, FileStreamMode::Read) == std::errc::device_or_resource_busy);
    CLST_ASSERT(native_handle_is_open(incoming));
    CLST_ASSERT(stream.get_handle() == handle && stream.tell_ex() == 4);
    stream.close_ex();
    CLST_ASSERT(!native_handle_is_open(handle));

    const auto invalid_mode = static_cast<FileStreamMode>(-1);
    CLST_ASSERT(stream.adopt(incoming, invalid_mode) == std::errc::invalid_argument);
    CLST_ASSERT(!stream.is_open() && native_handle_is_open(incoming));
#ifndef _WIN32
    CLST_ASSERT(stream.adopt(incoming, FileStreamMode::Write));
    CLST_ASSERT(!stream.is_open() && native_handle_is_open(incoming));
#endif
    {
        NativeBinaryFileStream reader;
        reader.adopt_ex(incoming, FileStreamMode::Read);
        CLST_ASSERT(reader.get_handle() == incoming && reader.tell_ex() == 0);
        CLST_ASSERT(reader.read_ex(data.data(), data.size()) == data.size());
        CLST_ASSERT(std::string_view(data.data(), data.size()) == "ab");
    }
    CLST_ASSERT(!native_handle_is_open(incoming));

#ifdef _WIN32
    CLST_ASSERT_THROW(stream.adopt_ex(INVALID_HANDLE_VALUE, FileStreamMode::Read), std::system_error);
    CLST_ASSERT(stream.adopt(nullptr, FileStreamMode::Read));
#else
    CLST_ASSERT_THROW(stream.adopt_ex(-1, FileStreamMode::Read), std::system_error);
    const auto append_handle = ::open(filename.c_str(), O_WRONLY | O_APPEND);
    CLST_ASSERT(append_handle != -1);
    CLST_ASSERT(stream.adopt(append_handle, FileStreamMode::Write));
    CLST_ASSERT(!stream.is_open() && native_handle_is_open(append_handle));
    CLST_ASSERT(!stream.adopt(append_handle, FileStreamMode::Append));
    CLST_ASSERT(stream.tell_ex() == 0);
    CLST_ASSERT(stream.write_ex("g", 1) == 1);
    stream.flush_ex();
    CLST_ASSERT(stream.tell_ex() == 7 && native_length(stream) == 7);
    stream.close_ex();
    CLST_ASSERT(!native_handle_is_open(append_handle));
#endif
}

void check_release(const std::string& filename)
{
    NativeBinaryFileStream::NativeHandle handle;
    {
        NativeBinaryFileStream stream;
        CLST_ASSERT(stream.release() == stream.get_handle());
        stream.open_ex(filename.c_str(), FileStreamMode::WriteExtended);
        const auto original = stream.get_handle();
        stream.write_ex("abcdef", 6);
        CLST_ASSERT(native_length(stream) == 0);

        handle = stream.release();
        CLST_ASSERT(handle == original && !stream.is_open());
        CLST_ASSERT(stream.release() == stream.get_handle());
    }
    CLST_ASSERT(native_handle_is_open(handle));
    {
        NativeBinaryFileStream stream(adopt_handle, handle, FileStreamMode::ReadExtended);
        CLST_ASSERT(stream.tell_ex() == 6 && stream.get_length_ex() == 6);
        stream.seek_ex(0);
        char first = 0;
        CLST_ASSERT(stream.read_ex(&first, 1) == 1 && first == 'a');
        CLST_ASSERT(stream.tell_ex() == 1 && native_position(stream) == 6);

        CLST_ASSERT(stream.release() == handle && !stream.is_open());
    }
    NativeBinaryFileStream stream(adopt_handle, handle, FileStreamMode::ReadExtended);
    CLST_ASSERT(stream.tell_ex() == 1);
    std::array<char, 5> remaining{};
    CLST_ASSERT(stream.read_ex(remaining.data(), remaining.size()) == remaining.size());
    CLST_ASSERT(std::string_view(remaining.data(), remaining.size()) == "bcdef");
}

template<typename Stream>
void check_modes_and_eof(const std::string& filename)
{
    Stream stream;
    CLST_ASSERT(!stream.is_open());
    CLST_ASSERT(!stream.tell() && stream.tell().error() == std::errc::bad_file_descriptor);
    CLST_ASSERT(stream.open(filename.c_str(), FileStreamMode::Read));
    CLST_ASSERT_THROW(stream.open_ex(filename.c_str(), FileStreamMode::ReadExtended), std::system_error);
    CLST_ASSERT(!stream.is_open());

    CLST_ASSERT(!stream.open(filename.c_str(), FileStreamMode::Write));
    CLST_ASSERT(stream.open(filename.c_str(), FileStreamMode::Read));
    const auto written = stream.write("abc", 3);
    CLST_ASSERT(written.bytes_transferred == 3 && !written.error);
    CLST_ASSERT(stream.tell() == 3 && stream.get_length() == 3);
    CLST_ASSERT(!stream.flush());
    CLST_ASSERT(!stream.close());

    stream.open_ex(filename.c_str());
    std::array<char, 5> data{};
    const auto read = stream.read(data.data(), data.size());
    CLST_ASSERT(read.bytes_transferred == 3 && !read.error);
    CLST_ASSERT(std::string_view(data.data(), read.bytes_transferred) == "abc");
    const auto eof = stream.read(data.data(), 1);
    CLST_ASSERT(eof.bytes_transferred == 0 && !eof.error);
    stream.seek_ex(-1, SeekFrom::End);
    try {
        stream.read_exact(data.data(), 2);
        CLST_ASSERT(false);
    } catch (const std::system_error& error) {
        CLST_ASSERT(error.code() == std::make_error_code(std::io_errc::stream));
    }
    CLST_ASSERT(stream.tell_ex() == 3);
    stream.read_exact(data.data(), 0);
    CLST_ASSERT_THROW(stream.write_exact("x", 1), std::system_error);
    stream.close_ex();

    stream.open_ex(filename.c_str(), FileStreamMode::ReadExtended);
    stream.read_exact(data.data(), 1);
    CLST_ASSERT(data.front() == 'a');
    stream.write_exact("B", 1);
    stream.read_exact(data.data(), 1);
    CLST_ASSERT(data.front() == 'c');
    stream.close_ex();

    stream.open_ex(filename.c_str(), FileStreamMode::Append);
    if constexpr (std::is_same_v<Stream, NativeBinaryFileStream>) {
        CLST_ASSERT(stream.tell_ex() == 3);
    }
    CLST_ASSERT(!stream.seek(0));
    CLST_ASSERT(stream.write_ex("d", 1) == 1);
    CLST_ASSERT(stream.get_length_ex() == 4 && stream.tell_ex() == 4);
    stream.close_ex();

    stream.open_ex(filename.c_str(), FileStreamMode::AppendExtended);
    if constexpr (std::is_same_v<Stream, NativeBinaryFileStream>) {
        CLST_ASSERT(stream.tell_ex() == 4);
    }
    stream.seek_ex(0);
    CLST_ASSERT(stream.read_ex(data.data(), 4) == 4);
    CLST_ASSERT(std::string_view(data.data(), 4) == "aBcd");
    stream.seek_ex(-2, SeekFrom::Current);
    CLST_ASSERT(stream.write_ex("e", 1) == 1);
    stream.seek_ex(-2, SeekFrom::End);
    CLST_ASSERT(stream.read_ex(data.data(), 2) == 2);
    CLST_ASSERT(std::string_view(data.data(), 2) == "de");
    stream.close_ex();

    stream.open_ex(filename.c_str(), FileStreamMode::WriteExtended);
    CLST_ASSERT(stream.get_length_ex() == 0);
    CLST_ASSERT(stream.write_ex("z", 1) == 1);
    stream.seek_ex(0);
    CLST_ASSERT(stream.read_ex(data.data(), 1) == 1 && data.front() == 'z');
    stream.close_ex();
}

void check_interrupted_exact_transfer()
{
    std::array<char, 3> data{};
    const auto interrupted = std::make_error_code(std::errc::interrupted);
    int calls = 0;
    const auto read = [&](void* buffer, std::size_t size) -> clst::TransferResult {
        auto* output = static_cast<char*>(buffer);
        switch (++calls) {
        case 1:
            return {0, interrupted};
        case 2:
            CLST_ASSERT(output == data.data() && size == 3);
            output[0] = 'a';
            return {1, interrupted};
        default:
            CLST_ASSERT(output == data.data() + 1 && size == 2);
            output[0] = 'b';
            output[1] = 'c';
            return {2, {}};
        }
    };

    CLST_ASSERT(clst::detail::read_file_exact(data.data(), data.size(), read) == data.size());
    CLST_ASSERT(calls == 3 && std::string_view(data.data(), data.size()) == "abc");
}

void check_cache_boundaries(const std::string& filename)
{
    constexpr std::size_t cache_size = 64 * 1024;
    std::vector<unsigned char> expected(2 * cache_size + 19);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        expected[i] = static_cast<unsigned char>(i % 251);
    }

    NativeBinaryFileStream stream(filename.c_str(), FileStreamMode::WriteExtended);
    stream.write_ex(expected.data(), 5);
    CLST_ASSERT(stream.tell_ex() == 5 && stream.get_length_ex() == 5);
    CLST_ASSERT(native_length(stream) == 0);
    stream.write_ex(expected.data() + 5, cache_size - 5);
    stream.write_ex(expected.data() + cache_size, cache_size);
    CLST_ASSERT(native_length(stream) == static_cast<OffsetType>(2 * cache_size));
    stream.write_ex(expected.data() + 2 * cache_size, expected.size() - 2 * cache_size);

    stream.seek_ex(0);
    std::vector<unsigned char> read(expected.size());
    stream.read_ex(read.data(), 5);
    stream.read_ex(read.data() + 5, cache_size - 2);
    stream.read_ex(read.data() + cache_size + 3, expected.size() - cache_size - 3);
    CLST_ASSERT(read == expected);

    stream.seek_ex(1);
    stream.read_ex(read.data(), 1);
    stream.flush_ex();
    CLST_ASSERT(native_position(stream) == 2);
    stream.close_ex();
}

void check_configurable_buffer(const std::string& filename)
{
    NativeBinaryFileStream stream;
    CLST_ASSERT(stream.get_buffer_size() == 64 * 1024);
    CLST_ASSERT(!stream.set_buffer_size(8));
    stream.open_ex(filename.c_str(), FileStreamMode::WriteExtended);
    stream.write_ex("abc", 3);
    CLST_ASSERT(native_length(stream) == 0);
    CLST_ASSERT(!stream.set_buffer_size(8));
    CLST_ASSERT(native_length(stream) == 0);
    stream.set_buffer_size_ex(4);
    CLST_ASSERT(stream.get_buffer_size() == 4);
    CLST_ASSERT(native_length(stream) == 3 && stream.tell_ex() == 3);

    stream.write_ex("def", 3);
    CLST_ASSERT(native_length(stream) == 3);
    stream.set_buffer_size_ex(0);
    CLST_ASSERT(stream.get_buffer_size() == 0);
    CLST_ASSERT(native_length(stream) == 6 && stream.tell_ex() == 6);
    stream.seek_ex(0);
    char c = 0;
    CLST_ASSERT(stream.read_ex(&c, 1) == 1 && c == 'a');
    CLST_ASSERT(native_position(stream) == 1);
    stream.seek_ex(0, SeekFrom::End);
    CLST_ASSERT(stream.write_ex("g", 1) == 1);
    CLST_ASSERT(native_length(stream) == 7 && stream.tell_ex() == 7);

    stream.set_buffer_size_ex(8);
    stream.seek_ex(0);
    CLST_ASSERT(stream.read_ex(&c, 1) == 1 && c == 'a');
    CLST_ASSERT(native_position(stream) == 7);
    stream.set_buffer_size_ex(2);
    CLST_ASSERT(stream.tell_ex() == 1 && native_position(stream) == 1);
    CLST_ASSERT(stream.read_ex(&c, 1) == 1 && c == 'b');
    CLST_ASSERT(stream.tell_ex() == 2 && native_position(stream) == 3);
    stream.close_ex();
    CLST_ASSERT(stream.get_buffer_size() == 2);

    stream.open_ex(filename.c_str(), FileStreamMode::WriteExtended);
    CLST_ASSERT(stream.get_buffer_size() == 2);
    stream.write_ex("x", 1);
    NativeBinaryFileStream moved(std::move(stream));
    CLST_ASSERT(!stream.is_open() && moved.get_buffer_size() == 2);
    CLST_ASSERT(moved.tell_ex() == 1 && native_length(moved) == 0);

    stream.set_buffer_size_ex(7);
    stream = std::move(moved);
    CLST_ASSERT(!moved.is_open() && stream.get_buffer_size() == 2);
    CLST_ASSERT(stream.tell_ex() == 1 && native_length(stream) == 0);
    stream.close_ex();
    stream.open_ex(filename.c_str());
    CLST_ASSERT(stream.get_buffer_size() == 2);
    CLST_ASSERT(stream.read_ex(&c, 1) == 1 && c == 'x');
}

void check_pipe_copy(const std::string& filename)
{
    NativeBinaryFileStream::NativeHandle read_handle;
#ifdef _WIN32
    HANDLE write_handle;
    CLST_ASSERT(CreatePipe(&read_handle, &write_handle, nullptr, 0));
    DWORD written = 0;
    CLST_ASSERT(WriteFile(write_handle, "pipe", 4, &written, nullptr) && written == 4);
    CLST_ASSERT(CloseHandle(write_handle));
#else
    int handles[2];
    CLST_ASSERT(::pipe(handles) == 0);
    read_handle = handles[0];
    CLST_ASSERT(::write(handles[1], "pipe", 4) == 4);
    CLST_ASSERT(::close(handles[1]) == 0);
#endif
    NativeBinaryFileStream input(adopt_handle, read_handle, FileStreamMode::Read);
    NativeBinaryFileStream output(filename.c_str(), FileStreamMode::WriteExtended);
    output.copy_from(input, 4);
    std::array<char, 4> data{};
    CLST_ASSERT(output.tell_ex() == 4 && output.read_at_ex(0, data.data(), data.size()) == data.size());
    CLST_ASSERT(std::string_view(data.data(), data.size()) == "pipe");
}

void check_native_copy(const std::string& input_filename, const std::string& output_filename)
{
    NativeBinaryFileStream input(input_filename.c_str(), FileStreamMode::WriteExtended);
    input.write_ex("0123456789", 10);
    input.seek_ex(0);
    std::array<char, 7> data{};
    CLST_ASSERT(input.read_ex(data.data(), 2) == 2);
    CLST_ASSERT(input.tell_ex() == 2 && native_position(input) == 10);

    NativeBinaryFileStream output(output_filename.c_str(), FileStreamMode::WriteExtended);
    output.write_ex(".", 1);
    CLST_ASSERT(output.tell_ex() == 1 && native_length(output) == 0);
    output.copy_from(input, 4);
    CLST_ASSERT(input.tell_ex() == 6 && output.tell_ex() == 5);
    CLST_ASSERT(output.read_at_ex(0, data.data(), 5) == 5);
    CLST_ASSERT(std::string_view(data.data(), 5) == ".2345");

    input.seek_ex(0, SeekFrom::End);
    try {
        output.copy_from(input, 1);
        CLST_ASSERT(false);
    } catch (const std::system_error& error) {
        CLST_ASSERT(error.code() == std::io_errc::stream);
    }
    CLST_ASSERT(input.tell_ex() == 10 && output.tell_ex() == 5);

    output.close_ex();
    output.open_ex(output_filename.c_str(), FileStreamMode::AppendExtended);
    output.seek_ex(0);
    input.seek_ex(6);
    output.copy_from(input, 2);
    CLST_ASSERT(input.tell_ex() == 8 && output.tell_ex() == 7);
    CLST_ASSERT(output.read_at_ex(0, data.data(), data.size()) == data.size());
    CLST_ASSERT(std::string_view(data.data(), data.size()) == ".234567");
}

void check_append_cache(const std::string& filename)
{
    NativeBinaryFileStream stream(filename.c_str(), FileStreamMode::Write);
    stream.write_ex("abc", 3);
    stream.close_ex();

    stream.open_ex(filename.c_str(), FileStreamMode::AppendExtended);
    stream.seek_ex(0);
    stream.write_ex("def", 3);
    CLST_ASSERT(stream.tell_ex() == 6 && stream.get_length_ex() == 6);
    CLST_ASSERT(native_length(stream) == 3);
    stream.flush_ex();
    CLST_ASSERT(native_length(stream) == 6 && native_position(stream) == 6);
    stream.seek_ex(0);
    std::array<char, 6> data{};
    stream.read_ex(data.data(), data.size());
    CLST_ASSERT(std::string_view(data.data(), data.size()) == "abcdef");
    stream.close_ex();
}

void check_positioned_io(const std::string& filename)
{
    NativeBinaryFileStream stream(filename.c_str(), FileStreamMode::WriteExtended, FileAccessPattern::Sequential);
    stream.write_ex("abcdefgh", 8);

    std::array<char, 5> data{};
    CLST_ASSERT(stream.read_at_ex(2, data.data(), 3) == 3);
    CLST_ASSERT(std::string_view(data.data(), 3) == "cde" && stream.tell_ex() == 8);
    CLST_ASSERT(native_length(stream) == 0);

    const auto pending_tail = stream.read_at(6, data.data(), 3);
    CLST_ASSERT(pending_tail.bytes_transferred == 2 && !pending_tail.error);
    CLST_ASSERT(std::string_view(data.data(), 2) == "gh");
    CLST_ASSERT(native_length(stream) == 8 && stream.tell_ex() == 8);

    stream.write_ex("ij", 2);
    CLST_ASSERT(stream.write_at_ex(8, "U", 1) == 1);
    CLST_ASSERT(native_length(stream) == 10 && stream.tell_ex() == 10);

    stream.seek_ex(0);
    CLST_ASSERT(stream.read_ex(data.data(), 1) == 1 && data.front() == 'a');
    const auto read_ahead_end = native_position(stream);
    CLST_ASSERT(read_ahead_end == 10);
    CLST_ASSERT(stream.read_at_ex(4, data.data(), 2) == 2);
    CLST_ASSERT(std::string_view(data.data(), 2) == "ef" && native_position(stream) == read_ahead_end);

    const auto disjoint = stream.write_at(read_ahead_end, "z", 1);
    CLST_ASSERT(disjoint.bytes_transferred == 1 && !disjoint.error);
    CLST_ASSERT(stream.tell_ex() == 1 && native_position(stream) == read_ahead_end);

    const auto written = stream.write_at(2, "XY", 2);
    CLST_ASSERT(written.bytes_transferred == 2 && !written.error);
    CLST_ASSERT(stream.tell_ex() == 1 && native_position(stream) == 1);
    CLST_ASSERT(stream.read_ex(data.data(), 4) == 4);
    CLST_ASSERT(std::string_view(data.data(), 4) == "bXYe");

    const auto tail = stream.read_at(8, data.data(), data.size());
    CLST_ASSERT(tail.bytes_transferred == 3 && !tail.error);
    CLST_ASSERT(std::string_view(data.data(), 3) == "Ujz");
    const auto eof = stream.read_at(11, data.data(), data.size());
    CLST_ASSERT(eof.bytes_transferred == 0 && !eof.error);
    try {
        (void)stream.read_at_ex(10, data.data(), 2);
        CLST_ASSERT(false);
    } catch (const std::system_error& error) {
        CLST_ASSERT(error.code() == std::io_errc::stream);
    }

    constexpr auto max_offset = std::numeric_limits<OffsetType>::max();
    CLST_ASSERT(stream.read_at(-1, data.data(), 1).error == std::errc::invalid_argument);
    CLST_ASSERT(stream.write_at(max_offset, "XY", 2).error);
    CLST_ASSERT(stream.tell_ex() == 5);
    stream.close_ex();

    CLST_ASSERT(stream.read_at(0, data.data(), 1).error == std::errc::bad_file_descriptor);
    CLST_ASSERT(stream.write_at(0, "x", 1).error == std::errc::bad_file_descriptor);
    stream.open_ex(filename.c_str(), FileStreamMode::AppendExtended);
    stream.seek_ex(1);
    const auto append_write = stream.write_at(0, "x", 1);
    CLST_ASSERT(append_write.bytes_transferred == 0 && append_write.error == std::errc::operation_not_supported);
    CLST_ASSERT_THROW(stream.write_at_ex(0, "x", 1), std::system_error);
    CLST_ASSERT(stream.tell_ex() == 1 && stream.get_length_ex() == 11);
    CLST_ASSERT(stream.read_at_ex(0, data.data(), 1) == 1 && data.front() == 'a');

    stream.write_ex("!", 1);
    CLST_ASSERT(stream.read_at_ex(11, data.data(), 1) == 1 && data.front() == '!');
    CLST_ASSERT(native_length(stream) == 11 && stream.tell_ex() == 12);
    CLST_ASSERT(stream.read_at_ex(10, data.data(), 2) == 2);
    CLST_ASSERT(std::string_view(data.data(), 2) == "z!");
    CLST_ASSERT(native_length(stream) == 12 && stream.tell_ex() == 12);
    stream.close_ex();
}

void check_file_management(const std::string& filename)
{
    NativeBinaryFileStream stream(filename.c_str(), FileStreamMode::WriteExtended);
    stream.write_ex("abcdef", 6);
    stream.resize_ex(3);
    CLST_ASSERT(stream.tell_ex() == 6);
    CLST_ASSERT(stream.get_length_ex() == 3 && native_length(stream) == 3);

    stream.seek_ex(0);
    std::array<char, 4> data{};
    CLST_ASSERT(stream.read_ex(data.data(), 1) == 1 && data.front() == 'a');
    CLST_ASSERT(!stream.resize(2));
    CLST_ASSERT(stream.tell_ex() == 1 && stream.get_length_ex() == 2);
    const auto tail = stream.read(data.data(), 2);
    CLST_ASSERT(tail.bytes_transferred == 1 && !tail.error && data.front() == 'b');

    stream.resize_ex(8);
    CLST_ASSERT(stream.tell_ex() == 2);
    CLST_ASSERT(stream.get_length_ex() == 8 && native_length(stream) == 8);
    CLST_ASSERT(stream.read_at_ex(0, data.data(), 2) == 2);
    CLST_ASSERT(std::string_view(data.data(), 2) == "ab");

    constexpr OffsetType reserve_size = 256 * 1024;
    stream.reserve_ex(reserve_size);
    CLST_ASSERT(!stream.reserve(reserve_size));
    CLST_ASSERT(!stream.reserve(32));
    CLST_ASSERT(stream.get_length_ex() == 8 && native_length(stream) == 8 && stream.tell_ex() == 2);

    stream.seek_ex(0, SeekFrom::End);
    stream.write_ex("x", 1);
    CLST_ASSERT(!stream.sync(FileSyncMode::Data));
    CLST_ASSERT(native_length(stream) == 9 && stream.tell_ex() == 9);
    stream.write_ex("y", 1);
    stream.sync_ex();
    CLST_ASSERT(native_length(stream) == 10 && stream.tell_ex() == 10);

    CLST_ASSERT(stream.resize(-1) == std::errc::invalid_argument);
    CLST_ASSERT_THROW(stream.reserve_ex(-1), std::system_error);
    stream.close_ex();
    CLST_ASSERT(stream.resize(1) == std::errc::bad_file_descriptor);
    CLST_ASSERT(stream.reserve(1) == std::errc::bad_file_descriptor);
    CLST_ASSERT(stream.sync() == std::errc::bad_file_descriptor);
    CLST_ASSERT(stream.prefetch(0, 1) == std::errc::bad_file_descriptor);

    CLST_ASSERT(!stream.open(filename.c_str(), FileStreamMode::Read, FileAccessPattern::Random));
    CLST_ASSERT(stream.resize(1));
    CLST_ASSERT(stream.reserve(16));
    stream.prefetch_ex(1, 0);
    const auto prefetch_error = stream.prefetch(0, 2);
#ifdef _WIN32
    CLST_ASSERT(prefetch_error == std::errc::operation_not_supported);
    CLST_ASSERT_THROW(stream.prefetch_ex(0, 2), std::system_error);
#else
    CLST_ASSERT(!prefetch_error);
    stream.prefetch_ex(1, 2);
#endif
    constexpr auto max_offset = std::numeric_limits<OffsetType>::max();
    CLST_ASSERT(stream.prefetch(-1, 1) == std::errc::invalid_argument);
    CLST_ASSERT(stream.prefetch(max_offset, 2));
    CLST_ASSERT(stream.tell_ex() == 0);
    CLST_ASSERT(stream.read_at_ex(8, data.data(), 2) == 2);
    CLST_ASSERT(std::string_view(data.data(), 2) == "xy");
    stream.close_ex();
}

void check_sparse_file(const std::string& filename)
{
    constexpr OffsetType offset = (OffsetType{1} << 32) + 7;
    NativeBinaryFileStream stream(filename.c_str(), FileStreamMode::WriteExtended);
#ifdef _WIN32
    DWORD output_size = 0;
    CLST_ASSERT(DeviceIoControl(stream.get_handle(), FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &output_size, nullptr));
#endif
    stream.seek_ex(offset);
    stream.write_ex("x", 1);
    CLST_ASSERT(stream.tell_ex() == offset + 1 && stream.get_length_ex() == offset + 1);
    stream.seek_ex(-2, SeekFrom::End);
    std::array<char, 2> data{};
    stream.read_ex(data.data(), data.size());
    CLST_ASSERT(data[0] == '\0' && data[1] == 'x');
    stream.close_ex();
}

void check_move_and_mixins(const std::string& filename, const std::string& other_filename)
{
    NativeBinaryFileStream stream(filename.c_str(), FileStreamMode::WriteExtended);
    stream.write_nums(std::uint16_t{1}, std::uint16_t{2});
    const auto handle = stream.get_handle();
    NativeBinaryFileStream moved(std::move(stream));
    CLST_ASSERT(!stream.is_open());
    CLST_ASSERT(moved.get_handle() == handle && moved.tell_ex() == 4);
    moved.patch_num(0, std::uint16_t{3});
    CLST_ASSERT(moved.tell_ex() == 4);
    moved.seek_ex(0);
    std::uint16_t first = 0;
    moved.read_num(first);
    CLST_ASSERT(first == 3);

    stream.open_ex(other_filename.c_str(), FileStreamMode::Write);
    stream.copy_from(moved, sizeof(std::uint16_t));
    CLST_ASSERT(stream.tell_ex() == 2);
    stream = std::move(moved);
    CLST_ASSERT(!moved.is_open());
    CLST_ASSERT(stream.get_handle() == handle && stream.tell_ex() == 4);
    stream.close_ex();

    BinaryFileStream output(other_filename.c_str());
    std::uint16_t value = 0;
    output.read_num(value);
    CLST_ASSERT(value == 2);
    NativeBinaryFileStream truncated_copy(filename.c_str(), FileStreamMode::Write);
    output.seek_ex(0);
    CLST_ASSERT_THROW(truncated_copy.copy_from(output, 3), std::system_error);
    CLST_ASSERT(truncated_copy.tell_ex() == 0);
}

#if defined(RLIMIT_FSIZE) && defined(SIGXFSZ)
class ScopedFileSizeLimit {
public:
    explicit ScopedFileSizeLimit(rlim_t limit)
    {
        struct sigaction ignore_signal{};
        ignore_signal.sa_handler = SIG_IGN;
        CLST_ASSERT(sigemptyset(&ignore_signal.sa_mask) == 0);
        CLST_ASSERT(sigaction(SIGXFSZ, &ignore_signal, &old_signal) == 0);
        CLST_ASSERT(getrlimit(RLIMIT_FSIZE, &old_limit) == 0);
        auto new_limit = old_limit;
        new_limit.rlim_cur = limit;
        CLST_ASSERT(setrlimit(RLIMIT_FSIZE, &new_limit) == 0);
    }

    ~ScopedFileSizeLimit()
    {
        CLST_ASSERT(setrlimit(RLIMIT_FSIZE, &old_limit) == 0);
        CLST_ASSERT(sigaction(SIGXFSZ, &old_signal, nullptr) == 0);
    }

private:
    struct rlimit old_limit{};
    struct sigaction old_signal{};
};

void check_flush_retry(const std::string& filename)
{
    NativeBinaryFileStream stream(filename.c_str(), FileStreamMode::WriteExtended);
    const auto handle = stream.get_handle();
    constexpr std::string_view expected = "abcdefghijklmnop";
    stream.write_ex(expected.data(), expected.size());
    {
        const ScopedFileSizeLimit limit(8);
        CLST_ASSERT(stream.flush() == std::errc::file_too_large);
        CLST_ASSERT(native_length(stream) == 8);
        CLST_ASSERT(stream.set_buffer_size(0) == std::errc::file_too_large);
        CLST_ASSERT(stream.get_buffer_size() == 64 * 1024);
        CLST_ASSERT(stream.close() == std::errc::file_too_large);
        CLST_ASSERT_THROW(stream.release(), std::system_error);
        CLST_ASSERT(stream.is_open() && stream.get_handle() == handle);
        CLST_ASSERT(stream.tell_ex() == static_cast<OffsetType>(expected.size()));
        CLST_ASSERT(stream.get_length_ex() == static_cast<OffsetType>(expected.size()));
    }
    stream.flush_ex();
    CLST_ASSERT(native_length(stream) == static_cast<OffsetType>(expected.size()));
    stream.seek_ex(0);
    std::array<char, expected.size()> read{};
    stream.read_ex(read.data(), read.size());
    CLST_ASSERT(std::string_view(read.data(), read.size()) == expected);
    stream.close_ex();

    constexpr std::size_t direct_limit = 64 * 1024;
    std::vector<unsigned char> large(direct_limit + 1, 'a');
    large.back() = 'z';
    stream.open_ex(filename.c_str(), FileStreamMode::WriteExtended);
    {
        const ScopedFileSizeLimit limit(direct_limit);
        try {
            stream.write_exact(large.data(), large.size());
            CLST_ASSERT(false);
        } catch (const std::system_error& error) {
            CLST_ASSERT(error.code() == std::errc::file_too_large);
        }
        CLST_ASSERT(native_length(stream) == static_cast<OffsetType>(direct_limit));
        CLST_ASSERT(stream.tell_ex() == static_cast<OffsetType>(direct_limit));
        stream.flush_ex();
    }
    stream.write_ex(&large.back(), 1);
    stream.flush_ex();
    CLST_ASSERT(native_length(stream) == static_cast<OffsetType>(large.size()));
    stream.seek_ex(-1, SeekFrom::End);
    unsigned char last = 0;
    stream.read_ex(&last, 1);
    CLST_ASSERT(last == large.back());
    stream.close_ex();
}

void check_positioned_write_error(const std::string& filename)
{
    NativeBinaryFileStream stream(filename.c_str(), FileStreamMode::WriteExtended);
    stream.seek_ex(2);
    constexpr std::string_view expected = "abcdef";
    {
        const ScopedFileSizeLimit limit(4);
        const auto written = stream.write_at(0, expected.data(), expected.size());
        CLST_ASSERT(written.bytes_transferred == 4 && !written.error);
        try {
            (void)stream.write_at_ex(0, expected.data(), expected.size());
            CLST_ASSERT(false);
        } catch (const std::system_error& error) {
            CLST_ASSERT(error.code() == std::errc::file_too_large);
        }
        CLST_ASSERT(stream.tell_ex() == 2 && native_length(stream) == 4);
    }

    CLST_ASSERT(stream.write_at_ex(4, expected.data() + 4, 2) == 2);
    std::array<char, expected.size()> data{};
    CLST_ASSERT(stream.read_at_ex(0, data.data(), data.size()) == data.size());
    CLST_ASSERT(std::string_view(data.data(), data.size()) == expected && stream.tell_ex() == 2);
}

#ifdef __linux__
void check_native_copy_error(const std::string& input_filename, const std::string& output_filename)
{
    NativeBinaryFileStream input(input_filename.c_str(), FileStreamMode::WriteExtended);
    input.write_ex("abcdef", 6);
    input.seek_ex(0);
    NativeBinaryFileStream output(output_filename.c_str(), FileStreamMode::Write);
    output.copy_from(input, 1);
    // The buffered fallback retains the byte in memory on filesystems without native copy support.
    if (native_length(output) == 0) {
        return;
    }

    const ScopedFileSizeLimit limit(4);
    try {
        output.copy_from(input, 5);
        CLST_ASSERT(false);
    } catch (const std::system_error& error) {
        CLST_ASSERT(error.code() == std::errc::file_too_large);
    }
    const auto copied = native_length(output);
    CLST_ASSERT(copied >= 1 && copied <= 4);
    CLST_ASSERT(output.tell_ex() == copied && input.tell_ex() == copied);
}
#endif
#endif

void check_truncated_mp4(const std::string& filename)
{
    constexpr std::array<unsigned char, 6> truncated_atom{0, 0, 0, 8, 'm', 'o'};
    NativeBinaryFileStream output(filename.c_str(), FileStreamMode::Write);
    output.write_ex(truncated_atom.data(), truncated_atom.size());
    output.close_ex();

    clst::mp4::Mp4Stream stream(filename.c_str());
    CLST_ASSERT_THROW(stream.parse_atom(), std::system_error);
    CLST_ASSERT(!stream.verify());
}

} // namespace

int binary_file_stream(int, char*[])
{
    const TempDirectory temp;
    check_adoption(temp.filename("adopt.bin"));
    check_release(temp.filename("release.bin"));
    check_modes_and_eof<BinaryFileStream>(temp.filename("legacy.bin"));
    check_modes_and_eof<NativeBinaryFileStream>(temp.filename("native.bin"));
    check_interrupted_exact_transfer();
    check_cache_boundaries(temp.filename("cache.bin"));
    check_configurable_buffer(temp.filename("buffer-size.bin"));
    check_native_copy(temp.filename("copy-input.bin"), temp.filename("copy-output.bin"));
    check_pipe_copy(temp.filename("copy-pipe.bin"));
    check_append_cache(temp.filename("append.bin"));
    check_positioned_io(temp.filename("positioned.bin"));
    check_file_management(temp.filename("management.bin"));
    check_sparse_file(temp.filename("sparse.bin"));
    check_move_and_mixins(temp.filename("move.bin"), temp.filename("copy.bin"));
#if defined(RLIMIT_FSIZE) && defined(SIGXFSZ)
    check_flush_retry(temp.filename("flush.bin"));
    check_positioned_write_error(temp.filename("positioned-write-error.bin"));
#ifdef __linux__
    check_native_copy_error(temp.filename("copy-error-input.bin"), temp.filename("copy-error-output.bin"));
#endif
#endif
    check_truncated_mp4(temp.filename("truncated.mp4"));
    return 0;
}
