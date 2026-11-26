#ifndef CLST_STREAMS_BINARY_STREAM_HPP
#define CLST_STREAMS_BINARY_STREAM_HPP

#include "clst/tags.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <new>
#include <stdexcept>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

namespace clst {

using OffsetType = std::int64_t;

enum class SeekFrom {
    Begin,
    Current,
    End
};

enum class Endian {
    LE,
    BE
};

static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big, "mixed endian is not supported");

inline constexpr Endian target_endian = (std::endian::native == std::endian::big) ? Endian::BE : Endian::LE;

struct StreamError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct StreamIoError : StreamError {
    using StreamError::StreamError;
};

// An operation can make progress and encounter an error in the same call.
struct TransferResult {
    std::size_t bytes_transferred = 0;
    std::error_code error;
};

// Tags for stream concepts.
// Implementors should inherit from these tags.
struct ReadStreamTag {};
struct WriteStreamTag {};
struct RWStreamTag : ReadStreamTag, WriteStreamTag {};

namespace detail {

inline void throw_stream_error(std::error_code error, const char* operation)
{
    if (error) {
        throw std::system_error(error, operation);
    }
}

template<typename T>
T stream_value(std::expected<T, std::error_code> result, const char* operation)
{
    if (!result) {
        throw std::system_error(result.error(), operation);
    }
    return std::move(*result);
}

template<typename T>
concept StreamPosition = std::same_as<T, OffsetType> || std::same_as<T, std::expected<OffsetType, std::error_code>>;

template<typename T>
concept StreamNumber = std::is_arithmetic_v<T> && std::same_as<T, std::remove_cv_t<T>>;

template<typename T, unsigned Bytes>
concept StreamNumberWidth = StreamNumber<T> && Bytes <= sizeof(T) && (!std::is_signed_v<T> || Bytes == sizeof(T));

} // namespace detail

// The mixins use the stream's non-throwing operations.
template<typename T>
concept BasicStream = requires(T& stream, OffsetType offset, SeekFrom from) {
    { stream.is_open() } noexcept -> std::same_as<bool>;
    { stream.seek(offset, from) } noexcept -> std::same_as<std::error_code>;
    { stream.tell() } noexcept -> detail::StreamPosition;
};

// Exact transfers report any progress and an error if they cannot complete.
template<typename T>
concept ReadStream = BasicStream<T> && std::derived_from<T, ReadStreamTag> &&
    requires(T& stream, void* buffer, std::size_t size) {
        { stream.read_exact(buffer, size) } noexcept -> std::same_as<TransferResult>;
    };

template<typename T>
concept WriteStream = BasicStream<T> && std::derived_from<T, WriteStreamTag> &&
    requires(T& stream, const void* buffer, std::size_t size) {
        { stream.write_exact(buffer, size) } noexcept -> std::same_as<TransferResult>;
    };

// Mixin class for readers
class ReadStreamMixin {
public:
    template<Endian endian = Endian::BE, typename T, unsigned Bytes = sizeof(T), ReadStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    [[nodiscard]] std::error_code read_num(this Self& self, T& dest) noexcept
    {
        // Decode into a temporary so an incomplete read leaves dest unchanged.
        using Storage = std::conditional_t<std::same_as<T, bool>, unsigned char[sizeof(T)], T>;
        Storage value{};
        constexpr unsigned offset = target_endian == Endian::LE ? 0 : sizeof(T) - Bytes;
        auto* bytes = reinterpret_cast<unsigned char*>(&value) + offset;
        if (const auto result = self.read_exact(bytes, Bytes); result.error) {
            return result.error;
        }
        if constexpr (std::same_as<T, bool>) {
            dest = std::any_of(bytes, bytes + Bytes, [](unsigned char byte) { return byte != 0; });
        } else {
            if constexpr (endian != target_endian) {
                std::reverse(bytes, bytes + Bytes);
            }
            dest = value;
        }
        return {};
    }

    template<Endian endian = Endian::BE, typename T, unsigned Bytes = sizeof(T), ReadStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    void read_num(this Self& self, Ex, T& dest)
    {
        detail::throw_stream_error(self.template read_num<endian, T, Bytes>(dest), "Read stream number");
    }

    template<Endian endian = Endian::BE, typename T, unsigned Bytes = sizeof(T), ReadStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    [[nodiscard]] std::expected<T, std::error_code> read_num(this Self& self) noexcept
    {
        T value{};
        if (const auto error = self.template read_num<endian, T, Bytes>(value)) {
            return std::unexpected(error);
        }
        return value;
    }

    template<Endian endian = Endian::BE, typename T, unsigned Bytes = sizeof(T), ReadStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    T read_num(this Self& self, Ex)
    {
        return detail::stream_value(self.template read_num<endian, T, Bytes>(), "Read stream number");
    }

    template<typename T, unsigned Bytes = sizeof(T), ReadStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    [[nodiscard]] std::error_code read_num(this Self& self, Endian endian, T& dest) noexcept
    {
        switch (endian) {
        case Endian::BE:
            return self.template read_num<Endian::BE, T, Bytes>(dest);
        case Endian::LE:
            return self.template read_num<Endian::LE, T, Bytes>(dest);
        }
        return std::make_error_code(std::errc::invalid_argument);
    }

    template<typename T, unsigned Bytes = sizeof(T), ReadStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    void read_num(this Self& self, Ex, Endian endian, T& dest)
    {
        detail::throw_stream_error(self.template read_num<T, Bytes>(endian, dest), "Read stream number");
    }

    template<typename T, unsigned Bytes = sizeof(T), ReadStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    [[nodiscard]] std::expected<T, std::error_code> read_num(this Self& self, Endian endian) noexcept
    {
        T value{};
        if (const auto error = self.template read_num<T, Bytes>(endian, value)) {
            return std::unexpected(error);
        }
        return value;
    }

    template<typename T, unsigned Bytes = sizeof(T), ReadStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    T read_num(this Self& self, Ex, Endian endian)
    {
        return detail::stream_value(self.template read_num<T, Bytes>(endian), "Read stream number");
    }

    // On failure, only the numbers read before the failing transfer are assigned.
    template<Endian endian = Endian::BE, detail::StreamNumber... Ts, ReadStream Self>
    [[nodiscard]] std::error_code read_nums(this Self& self, Ts&... args) noexcept
    {
        std::error_code error;
        (void)((!(error = self.template read_num<endian>(args))) && ...);
        return error;
    }

    template<Endian endian = Endian::BE, detail::StreamNumber... Ts, ReadStream Self>
    void read_nums(this Self& self, Ex, Ts&... args)
    {
        detail::throw_stream_error(self.template read_nums<endian>(args...), "Read stream numbers");
    }

    template<Endian endian = Endian::BE, detail::StreamNumber... Ts, ReadStream Self>
    [[nodiscard]] std::expected<std::tuple<Ts...>, std::error_code> read_nums(this Self& self) noexcept
    {
        std::tuple<Ts...> values;
        if (const auto error = std::apply([&self](auto&... args) {
                return self.template read_nums<endian>(args...);
            }, values)) {
            return std::unexpected(error);
        }
        return values;
    }

    template<Endian endian = Endian::BE, detail::StreamNumber... Ts, ReadStream Self>
    std::tuple<Ts...> read_nums(this Self& self, Ex)
    {
        return detail::stream_value(self.template read_nums<endian, Ts...>(), "Read stream numbers");
    }

    template<detail::StreamNumber... Ts, ReadStream Self>
    [[nodiscard]] std::error_code read_nums(this Self& self, Endian endian, Ts&... args) noexcept
    {
        switch (endian) {
        case Endian::BE:
            return self.template read_nums<Endian::BE>(args...);
        case Endian::LE:
            return self.template read_nums<Endian::LE>(args...);
        }
        return std::make_error_code(std::errc::invalid_argument);
    }

    template<detail::StreamNumber... Ts, ReadStream Self>
    void read_nums(this Self& self, Ex, Endian endian, Ts&... args)
    {
        detail::throw_stream_error(self.read_nums(endian, args...), "Read stream numbers");
    }

    template<detail::StreamNumber... Ts, ReadStream Self>
    [[nodiscard]] std::expected<std::tuple<Ts...>, std::error_code> read_nums(this Self& self, Endian endian) noexcept
    {
        switch (endian) {
        case Endian::BE:
            return self.template read_nums<Endian::BE, Ts...>();
        case Endian::LE:
            return self.template read_nums<Endian::LE, Ts...>();
        }
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }

    template<detail::StreamNumber... Ts, ReadStream Self>
    std::tuple<Ts...> read_nums(this Self& self, Ex, Endian endian)
    {
        return detail::stream_value(self.template read_nums<Ts...>(endian), "Read stream numbers");
    }
};

// Mixin class for writer streams
class WriteStreamMixin {
public:
    // Counts bytes accepted by the destination. The source can advance farther on failure.
    template<ReadStream Src, WriteStream Self>
    [[nodiscard]] TransferResult copy_from(this Self& self, Src& in, std::size_t n) noexcept
    {
        if (!(in.is_open() && self.is_open())) {
            return {0, std::make_error_code(std::errc::bad_file_descriptor)};
        }

        std::size_t copied = 0;
        // A partial native copy leaves both cursors at the first uncopied byte.
        if constexpr (requires { { self.copy_from_native(in, n) } noexcept -> std::same_as<TransferResult>; }) {
            const auto result = self.copy_from_native(in, n);
            if (result.error) {
                return result;
            }
            copied = result.bytes_transferred;
        }
        if (copied == n) {
            return {copied, {}};
        }

        constexpr std::size_t max_buffer_size = 4 * 1024 * 1024;
        const auto buffer_size = std::min(n - copied, max_buffer_size);
        const std::unique_ptr<unsigned char[]> buffer(new (std::nothrow) unsigned char[buffer_size]);
        if (!buffer) {
            return {copied, std::make_error_code(std::errc::not_enough_memory)};
        }

        while (copied < n) {
            const auto chunk_size = std::min(n - copied, buffer_size);
            if (const auto result = in.read_exact(buffer.get(), chunk_size); result.error) {
                return {copied, result.error};
            }
            const auto result = self.write_exact(buffer.get(), chunk_size);
            copied += result.bytes_transferred;
            if (result.error) {
                return {copied, result.error};
            }
        }
        return {copied, {}};
    }

    template<ReadStream Src, WriteStream Self>
    void copy_from(this Self& self, Ex, Src& in, std::size_t n)
    {
        const auto result = self.copy_from(in, n);
        if (result.error == std::errc::bad_file_descriptor && !(in.is_open() && self.is_open())) {
            throw StreamError("Stream not open");
        }
        detail::throw_stream_error(result.error, "Copy stream range");
    }

    // Attempt to restore the original position even if a write fails.
    template<WriteStream Self>
    [[nodiscard]] std::error_code patch_bytes(this Self& self, OffsetType offset, const void* buffer, std::size_t n) noexcept
    {
        return patch_at(self, offset, [&] { return self.write_exact(buffer, n).error; });
    }

    template<WriteStream Self>
    void patch_bytes(this Self& self, Ex, OffsetType offset, const void* buffer, std::size_t n)
    {
        detail::throw_stream_error(self.patch_bytes(offset, buffer, n), "Patch stream bytes");
    }

    template<Endian endian = Endian::BE, typename T, unsigned Bytes = sizeof(T), WriteStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    [[nodiscard]] std::error_code write_num(this Self& self, const T& src) noexcept
    {
        constexpr unsigned offset = target_endian == Endian::LE ? 0 : sizeof(T) - Bytes;
        const auto* bytes = reinterpret_cast<const unsigned char*>(&src) + offset;
        if constexpr (endian != target_endian) {
            std::array<unsigned char, sizeof(T)> reversed;
            std::reverse_copy(bytes, bytes + Bytes, reversed.begin());
            return self.write_exact(reversed.data(), Bytes).error;
        } else {
            return self.write_exact(bytes, Bytes).error;
        }
    }

    template<Endian endian = Endian::BE, typename T, unsigned Bytes = sizeof(T), WriteStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    void write_num(this Self& self, Ex, const T& src)
    {
        detail::throw_stream_error(self.template write_num<endian, T, Bytes>(src), "Write stream number");
    }

    template<Endian endian = Endian::BE, detail::StreamNumber... Ts, WriteStream Self>
    [[nodiscard]] std::error_code write_nums(this Self& self, const Ts&... args) noexcept
    {
        std::error_code error;
        (void)((!(error = self.template write_num<endian>(args))) && ...);
        return error;
    }

    template<Endian endian = Endian::BE, detail::StreamNumber... Ts, WriteStream Self>
    void write_nums(this Self& self, Ex, const Ts&... args)
    {
        detail::throw_stream_error(self.template write_nums<endian>(args...), "Write stream numbers");
    }

    template<Endian endian = Endian::BE, typename T, unsigned Bytes = sizeof(T), WriteStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    [[nodiscard]] std::error_code patch_num(this Self& self, OffsetType offset, const T& src) noexcept
    {
        return patch_at(self, offset, [&] { return self.template write_num<endian, T, Bytes>(src); });
    }

    template<Endian endian = Endian::BE, typename T, unsigned Bytes = sizeof(T), WriteStream Self>
    requires detail::StreamNumberWidth<T, Bytes>
    void patch_num(this Self& self, Ex, OffsetType offset, const T& src)
    {
        detail::throw_stream_error(self.template patch_num<endian, T, Bytes>(offset, src), "Patch stream number");
    }

private:
    template<typename Self, typename Write>
    static std::error_code patch_at(Self& stream, OffsetType offset, Write write) noexcept
    {
        const std::expected<OffsetType, std::error_code> mark = stream.tell();
        if (!mark) {
            return mark.error();
        }
        if (const auto error = stream.seek(offset, SeekFrom::Begin)) {
            return error;
        }
        const auto error = write();
        const auto restore_error = stream.seek(*mark, SeekFrom::Begin);
        return error ? error : restore_error;
    }
};

// Mixin class for RW streams.
class RWStreamMixin : public ReadStreamMixin, public WriteStreamMixin {};

} // namespace clst

#endif /* CLST_STREAMS_BINARY_STREAM_HPP */
