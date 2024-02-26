#ifndef CLST_COMMON_EXCEPTIONS_H
#define CLST_COMMON_EXCEPTIONS_H

#include <stdexcept>

namespace clst::error
{

class ExceptionBase : public std::runtime_error {
public:
    virtual ~ExceptionBase() = default;
protected:
    using runtime_error::runtime_error;
};

class IoError : public ExceptionBase {
public:
    using ExceptionBase::ExceptionBase;
};

class InvalidArgument : public ExceptionBase {
public:
    using ExceptionBase::ExceptionBase;
};

class NumericalError : public ExceptionBase {
public:
    using ExceptionBase::ExceptionBase;
};

class BadNumCast final : public NumericalError {
    using NumericalError::NumericalError;
};

} // namespace clst::error


#endif /* CLST_COMMON_EXCEPTIONS_H */
