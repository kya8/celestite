#ifndef CLST_CONTAINER_RING_BUFFER_H
#define CLST_CONTAINER_RING_BUFFER_H

#include <memory>
#include <stdexcept>

namespace clst {

template <typename T, typename Allocator = std::allocator<T>>
class RingBuffer {
private:
    using alloc_traits = std::allocator_traits<Allocator>;
public:
    using size_type       = typename alloc_traits::size_type;
    using difference_type = typename alloc_traits::difference_type;
    using value_type      = T;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = typename alloc_traits::pointer;
    using const_pointer   = typename alloc_traits::const_pointer;
    //FIXME: iterator = ...;
    using allocator_type  = Allocator;

    RingBuffer(size_type max, const allocator_type& alloc = allocator_type{})
    : alloc_{alloc}, data_(alloc_traits::allocate(alloc_, max)), first_(0), count_(0), max_(max) {}

    ~RingBuffer() noexcept {
        clear();
        alloc_traits::deallocate(alloc_, data_, max_);
    }

private:
    template<class V>
    bool push_impl(V&& value) {
        if (count_ == max_) return false;
        alloc_traits::construct(alloc_, data_ + (first_+count_)%max_, std::forward<V>(value));
        ++count_;
        return true;
    }
    template<class V>
    void push_overwrite_impl(V&& value) {
        if (count_ == max_) pop();
        alloc_traits::construct(alloc_, data_ + (first_+count_)%max_, std::forward<V>(value));
        ++count_;
    }

public:
    bool push(const T& value) {
        return push_impl(value);
    }
    bool push(T&& value) {
        return push_impl(std::move(value));
    }
    void push_overwrite(const T& value) {
        return push_overwrite_impl(value);
    }
    void push_overwrite(T&& value) {
        return push_overwrite_impl(std::move(value));
    }
    template <typename ...Args>
    auto& emplace(Args ...args) {
        if (count_ == max_) pop();
        const auto ptr = data_ + (first_+count_) % max_;
        alloc_traits::construct(alloc_, ptr, std::forward<Args>(args)...);
        ++count_;
        return *ptr;
    }

    void pop() noexcept { // UB if empty
        alloc_traits::destroy(alloc_, data_+first_);
        first_ = (first_+1) % max_;
        --count_;
    }
    void pop_back() noexcept {
        alloc_traits::destroy(alloc_, data_ + (first_+count_-1) % max_);
        --count_;
    }

    auto& operator[](size_type i) const noexcept {
        return data_[(first_+i) % max_];
    }
    auto& operator[](size_type i) noexcept {
        return data_[(first_+i) % max_];
    }
    auto& at(size_type i) {
        if (i >= count_) throw std::out_of_range("RingBuffer subscription out of range");
        return (*this)[i];
    }
    auto& at(size_type i) const {
        if (i >= count_) throw std::out_of_range("RingBuffer subscription out of range");
        return (*this)[i];
    }
    auto& front() const noexcept { return (*this)[0]; }
    auto& front()       noexcept { return (*this)[0]; }
    auto& back()  const noexcept { return (*this)[count_ - 1]; }
    auto& back()        noexcept { return (*this)[count_ - 1]; }

    bool empty() const noexcept { return count_ == 0; }
    size_type size()     const noexcept { return count_; }
    size_type capacity() const noexcept { return max_; }

    void clear() noexcept {
        for (auto i=first_; i<first_ + count_; ++i) {
            alloc_traits::destroy(alloc_, &data_[i % max_]);
            --count_;
        }
    }

private:
    allocator_type alloc_;
    T* data_ = nullptr;
    size_type first_;
    size_type count_;
    size_type max_;
};

} // namespace clst

#endif /* CLST_CONTAINER_RING_BUFFER_H */
