#ifndef CLST_COMMON_SCOPE_GUARD_HPP
#define CLST_COMMON_SCOPE_GUARD_HPP

#include <utility> // std::forward

namespace clst::util {

template<class F>
class ScopeGuard {
public:
    constexpr ScopeGuard(F&& f) noexcept(noexcept(F(std::forward<F>(f)))) : func_(std::forward<F>(f)) {}
    ~ScopeGuard() {
        std::forward<F>(func_)();
    }

    // ScopeGuard should not be copied or moved.
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

private:
    F func_;
};

// CTAD guide
// rvalue -> value(move in); lvalue -> lvalue ref
template<typename T>
ScopeGuard(T&&) -> ScopeGuard<T>;

} // namespace clst::util


#endif /* CLST_COMMON_SCOPE_GUARD_HPP */
