#ifndef CLST_SCOPE_GUARD_HPP
#define CLST_SCOPE_GUARD_HPP

#include <utility> // std::forward

namespace clst {

template<class F>
class ScopeGuard {
public:
    ScopeGuard(F&& f) noexcept(noexcept(F(std::forward<F>(f)))) : func_(std::forward<F>(f)) {}
    ~ScopeGuard() {
        std::forward<F>(func_)();
    }

private:
    F func_;
};

// CTAD guides
// Store a reference for lvalue argument.
template<typename T>
ScopeGuard(T&) -> ScopeGuard<T&>;
// For rvalue argument we want to move it in.
template<typename T>
ScopeGuard(T&&) -> ScopeGuard<T>;

} // namespace clst

#endif // CLST_SCOPE_GUARD_HPP
