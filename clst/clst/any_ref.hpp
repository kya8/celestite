#ifndef CLST_ANY_REF_HPP
#define CLST_ANY_REF_HPP

#include <exception>
#include <type_traits>

namespace clst {

namespace detail {

template<typename T>
inline int any_ref_helper{};

} // namespace detail

class BadAnyRefCast : std::exception {
public:
    const char* what() const noexcept override {
        return "Bad AnyRef cast.";
    }
};

class AnyRef {
public:
    template<typename T,
             // Allow copying
             typename = std::enable_if_t<!std::is_same_v<AnyRef, std::remove_cv_t<T>>>
             >
    AnyRef(T& val) noexcept : data_(const_cast<void*>(static_cast<const void*>(&val))),
    type_ptr_(&detail::any_ref_helper<T>)
    {}

    template<typename T>
    T& cast() const {
        if (type_ptr_ == &detail::any_ref_helper<T>) {
            return *static_cast<T*>(data_);
        }
        throw BadAnyRefCast{};
    }

    template<typename T>
    T* cast_if() const noexcept {
        if (type_ptr_ == &detail::any_ref_helper<T>) {
            return static_cast<T*>(data_);
        }
        return nullptr;
    }

    // to_owned() -> std::any

private:
    void* data_;
    const int* type_ptr_;
};

} // namespace clst

#endif // CLST_ANY_REF_HPP
