#include "clst/error.hpp"
#include <cassert>

namespace clst {

namespace detail {

ArcString::ArcString(const char* str) : rep_(new ArcStringRep{str}) {}
ArcString::ArcString(const std::string& str) : rep_(new ArcStringRep{str}) {}
ArcString::ArcString(std::string&& str) noexcept : rep_(new ArcStringRep{std::move(str)}) {}

ArcString::~ArcString() noexcept
{
    release();
}

ArcString::ArcString(const ArcString& rhs) noexcept : rep_(rhs.rep_)
{
    rep_->ref_cnt.fetch_add(1, std::memory_order_relaxed);
}
ArcString&
ArcString::operator=(const ArcString& rhs) noexcept
{
    release();
    rep_ = rhs.rep_;
    rep_->ref_cnt.fetch_add(1, std::memory_order_relaxed);
    return *this;
}

ArcString::ArcString(ArcString&& rhs) noexcept : rep_(rhs.rep_)
{
    rhs.rep_ = nullptr;
}
ArcString&
ArcString::operator=(ArcString&& rhs) noexcept
{
    release();
    rep_ = rhs.rep_;
    rhs.rep_ = nullptr;
    return *this;
}

bool
ArcString::empty() const noexcept
{
    return rep_ == nullptr;
}

ArcString::operator bool() const noexcept
{
    return !empty();
}

const std::string&
ArcString::string() const noexcept
{
    assert(!empty() && "This ArcString is empty!");
    return rep_->str;
}

void
ArcString::release() noexcept
{
    if (rep_ && rep_->ref_cnt.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        // I'm the last handle
        delete rep_;
    }
    rep_ = nullptr;
}

} // namespace detail

RuntimeError::RuntimeError(const char* msg) : msg_(msg) {}
RuntimeError::RuntimeError(const std::string& msg) : msg_(msg) {}
RuntimeError::RuntimeError(std::string&& msg) noexcept : msg_(std::move(msg)) {}

const char*
RuntimeError::what() const noexcept
{
    return msg_.string().c_str();
}

const std::string&
RuntimeError::string() const noexcept
{
    return msg_.string();
}

SystemError::SystemError(int system_error_code) noexcept : ec_(std::system_category().default_error_condition(system_error_code)) {}
SystemError::SystemError(std::errc ec) noexcept : ec_(ec) {}

const std::error_condition&
SystemError::get_error() const noexcept
{
    return ec_;
}

const char*
SystemError::what() const noexcept
{
    return ec_.message().c_str();
}

} // namespace clst
