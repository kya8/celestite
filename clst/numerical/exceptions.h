#ifndef CLST_NUMERICAL_EXCEPTIONS_H
#define CLST_NUMERICAL_EXCEPTIONS_H

#include "clst/common/exceptions.h"

namespace clst::error
{

class NumericalError : public ExceptionBase {
public:
    using ExceptionBase::ExceptionBase;
};

class BadNumCast final : public NumericalError {
    using NumericalError::NumericalError;
};

} // namespace clst::numerical


#endif /* CLST_NUMERICAL_EXCEPTIONS_H */
