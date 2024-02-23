#ifndef CLST_COMMON_EXCEPTIONS_H
#define CLST_COMMON_EXCEPTIONS_H

#include <stdexcept>

namespace clst::errors
{

class exception_base : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    virtual ~exception_base() = default;
};

class io_error : public exception_base {
public:
    using exception_base::exception_base;
};

class numerical_error : public exception_base {
public:
    using exception_base::exception_base;
};

class num_cast_error final : public numerical_error {
    using numerical_error::numerical_error;
};

} // namespace clst::errors


#endif /* CLST_COMMON_EXCEPTIONS_H */
