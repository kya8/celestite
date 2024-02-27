#ifndef CLST_CONCURRENCY_EXCEPTIONS_H
#define CLST_CONCURRENCY_EXCEPTIONS_H

#include "clst/common/exceptions.h"

namespace clst::error
{

class ThreadPoolError : public RuntimeError {
public:
    using RuntimeError::RuntimeError;
};

} // namespace clst::error


#endif /* CLST_CONCURRENCY_EXCEPTIONS_H */
