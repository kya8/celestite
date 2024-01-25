#ifndef CLST_COMMON_EXCEPTIONS_H
#define CLST_COMMON_EXCEPTIONS_H

#include <stdexcept>

namespace clst::errors
{

class exception_base : public std::runtime_error {
public:
    explicit exception_base() : runtime_error("") {}
    using runtime_error::runtime_error;
    virtual ~exception_base() = default;
};

class io_error : public exception_base {
public:
    using exception_base::exception_base;
    virtual ~io_error() = default;
};

class numerical_error : public exception_base {
public:
    using exception_base::exception_base;
    virtual ~numerical_error() = default;
};

class bad_num_cast final : public numerical_error {
public:
    using numerical_error::numerical_error;
};

} // namespace clst::errors


#endif /* CLST_COMMON_EXCEPTIONS_H */
