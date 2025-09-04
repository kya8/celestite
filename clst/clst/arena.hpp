#ifndef CLST_ARENA_HPP
#define CLST_ARENA_HPP

#include <cstddef> // ptrdiff_t
#include <cstdint> // intptr_t

namespace clst {

class Arena {
public:
    Arena(void* begin, std::ptrdiff_t length): cur_(static_cast<unsigned char*>(begin)), end_(cur_ + length) {}
    Arena(void* begin, void* end): cur_(static_cast<unsigned char*>(begin)), end_(static_cast<unsigned char*>(end)) {}

    // Not copy-able or move-able
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    template<typename T>
    void* allocate() noexcept
    {
        return allocate(static_cast<std::ptrdiff_t>(sizeof(T)), static_cast<std::ptrdiff_t>(alignof(T)));
    }

    void* allocate(std::ptrdiff_t size, std::ptrdiff_t alignment) noexcept
    {
        // alignment is 2^N, this essentially computes: pad = (2^N - (cur % 2^N)) % 2^N
        const auto pad = static_cast<std::ptrdiff_t>(-reinterpret_cast<std::uintptr_t>(cur_) & (alignment - 1));
        if (end_ - cur_ < pad + size) {
            return nullptr;
        }
        const auto p = cur_ + pad;
        cur_ += pad + size;
        return p;
    }
    
    //void* align(std::ptrdiff_t alignment) noexcept;

private:
    //unsigned char* beg_;
    unsigned char* cur_;
    unsigned char* end_;
};

} // namespace clst

#endif /* CLST_ARENA_HPP */
