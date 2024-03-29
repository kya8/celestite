#ifndef CLST_UTIL_THROWING_PTR_H
#define CLST_UTIL_THROWING_PTR_H

#include <clst/common/exceptions.h>
#include <type_traits>

namespace clst::error {

class NullPtrAccess : RuntimeError {
public:
    using RuntimeError::RuntimeError;
};

} // namespace clst::error

namespace clst {

template <typename T>
class throwing_ptr {
    static_assert(!std::is_void_v<T>, "void pointer is not supported");
public:
    constexpr throwing_ptr(T* ptr) noexcept : ptr_(ptr) {}
    constexpr throwing_ptr() noexcept = default;

    constexpr auto& ptr() const noexcept { return ptr_; }
    constexpr auto& ptr() noexcept { return ptr_; }

    constexpr auto& unsafeGet() { return *ptr_; }
    constexpr auto& operator*() {
        if (ptr_) return *ptr_;
        throw NullPtrAccess{};
    }
    constexpr auto operator->() {
        if (ptr_) return ptr_;
        throw NullPtrAccess{};
    }

    template <typename ...Ts>
    constexpr decltype(auto) operator()(Ts&& ...Args) {
        if (ptr_) return ptr_(std::forward<Ts>(Args)...);
        throw NullPtrAccess{};
    }

    // pointer arithmetics are not provided... Use ptr()

private:
    T* ptr_ = nullptr;
};

} // namespace clst


#endif /* CLST_UTIL_THROWING_PTR_H */
