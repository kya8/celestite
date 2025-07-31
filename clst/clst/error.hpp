#ifndef CLST_ERROR_HPP
#define CLST_ERROR_HPP

#include <exception>
#include <atomic>
#include <cstddef>
#include <string>
#include <system_error>


namespace clst {

namespace detail {

// Storage for ArcString
struct ArcStringRep {
    // Common implementation of ref-counted string for std::runtime_error
    // is a contiguous buffer holding a atomic counter immediately followed by a null-terminated string,
    // which sidesteps one additional memory allocation, but looks UB to me.
    // Here we simply store an std::string with an atomic counter in the "control block".
    const std::string str; // Not meant to be mutable.
    std::atomic<std::ptrdiff_t> ref_cnt = 1;

    // Not meant to be copied or moved by myself.
    ArcStringRep(const ArcStringRep&) = delete;
    ArcStringRep& operator=(const ArcStringRep&) = delete;
};

// Ref-counted, immutable string. For use with clst::Error only
class ArcString {
public:
    ArcString(const char* str);
    ArcString(const std::string& str);
    ArcString(std::string&& str) noexcept;
    ~ArcString() noexcept;
    // Copy
    ArcString(const ArcString& rhs) noexcept;
    ArcString& operator=(const ArcString& rhs) noexcept;
    // Move
    ArcString(ArcString&& rhs) noexcept;
    ArcString& operator=(ArcString&& rhs) noexcept;

    bool empty() const noexcept;
    explicit operator bool() const noexcept;

    /**
     * @pre `!empty()` 
     */
    const std::string& string() const noexcept;

private:
    ArcStringRep* rep_; // Can become null after releasing ownership.

    // Release ownership, decrease the ref counter, free the storage block if required.
    void release() noexcept;
};

} // namespace detail

class Error : public std::exception {
public:
    const char* what() const noexcept override {
        return "clst::Error";
    }
};

class RuntimeError : public Error {
public:
    RuntimeError(const char* msg);
    RuntimeError(const std::string& msg);
    RuntimeError(std::string&& msg) noexcept;
    // Copy (no throw)
    RuntimeError(const RuntimeError&) noexcept = default;
    RuntimeError& operator=(const RuntimeError&) noexcept = default;
    // Disable implicit move, to prevent empty ArcString.
    RuntimeError(RuntimeError&&) = delete;
    RuntimeError& operator=(RuntimeError&&) = delete;

    const char* what() const noexcept override;
    const std::string& string() const noexcept;

private:
    detail::ArcString msg_;
};

namespace detail {

// Helper class, so that error_code is constructed before the message.
class SystemErrorBase : public RuntimeError {
protected:
    SystemErrorBase(std::error_code ec);

    std::error_code ec_;
};

} // namespace detail

class SystemError : public detail::SystemErrorBase {
public:
    // Construct with system-specific error code value.
    SystemError(int system_error_code) noexcept;
    SystemError(int code, const std::error_category& ecat) noexcept;
    SystemError(std::errc ec) noexcept;

    const std::error_code& get_error() const noexcept;
    //const char* what() const noexcept override;

    // Throw last system error from errno/GetLastError().
    static void throw_last();
};

} // namespace clst

#endif // CLST_ERROR_HPP
