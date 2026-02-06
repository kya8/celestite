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

    // ScopeGuard should not be copied.
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    // A moved-from ScopeGuard still runs the callabale at destruction,
    // unless we track it with additional state.
    // Disable move.
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

private:
    F func_;
};

// CTAD guide
// rvalue -> value(move in), lvalue -> lvalue ref
template<typename T>
ScopeGuard(T&&) -> ScopeGuard<T>;

} // namespace clst

#endif // CLST_SCOPE_GUARD_HPP
