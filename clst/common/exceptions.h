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

class RuntimeError : public ExceptionBase {
public:
    using ExceptionBase::ExceptionBase;
};

class IoError : public ExceptionBase {
public:
    using ExceptionBase::ExceptionBase;
};

class InvalidArgument : public ExceptionBase {
public:
    using ExceptionBase::ExceptionBase;
};

} // namespace clst::error


#endif /* CLST_COMMON_EXCEPTIONS_H */
