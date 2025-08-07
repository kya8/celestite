#ifndef CLST_RING_BUFFER_HPP
#define CLST_RING_BUFFER_HPP

#include <memory>
#include <stdexcept>
#include "clst/utils.hpp" // CompressPair

//FIXME: allocator

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
    : alloc_and_data_{alloc, {}}, first_(0), count_(0), max_(max) {
        alloc_and_data_.second() = alloc_traits::allocate(alloc_(), max);
    }

    ~RingBuffer() noexcept {
        clear();
        alloc_traits::deallocate(alloc_(), data_(), max_);
    }

    RingBuffer(RingBuffer&& rhs) noexcept :
    alloc_and_data_{std::move(rhs.alloc_and_data_)}, // ensure moving the allocator. The pointer is copied.
    first_{rhs.first_},
    count_{rhs.count_},
    max_{rhs.max_}
    {
        rhs.alloc_and_data_.second() = nullptr;
        rhs.count_ = 0;
        rhs.max_ = 0;
        rhs.first_ = 0;
        // rhs shouldn't be used anymore
    }

    RingBuffer& operator=(RingBuffer&& rhs) noexcept {
        clear();
        alloc_traits::deallocate(alloc_(), data_(), max_);

        alloc_and_data_ = std::move(rhs.alloc_and_data_); // ensure moving the allocator
        first_ = rhs.first_;
        count_ = rhs.count_;
        max_ = rhs.max_;
        rhs.alloc_and_data_.second() = nullptr;
        rhs.count_ = 0;
        rhs.max_ = 0;
        rhs.first_ = 0;
        // rhs shouldn't be used anymore
        return *this;
    }

private:
    template<class V>
    bool push_impl(V&& value) {
        if (count_ == max_) return false;
        alloc_traits::construct(alloc_(), data_() + (first_+count_)%max_, std::forward<V>(value));
        ++count_;
        return true;
    }
    template<class V>
    void push_overwrite_impl(V&& value) {
        if (count_ == max_) pop();
        alloc_traits::construct(alloc_(), data_() + (first_+count_)%max_, std::forward<V>(value));
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
    auto& emplace(Args&& ...args) {
        if (count_ == max_) pop();
        const auto ptr = data_() + (first_+count_) % max_;
        alloc_traits::construct(alloc_(), ptr, std::forward<Args>(args)...);
        ++count_;
        return *ptr;
    }

    void pop() noexcept { // UB if empty
        alloc_traits::destroy(alloc_(), data_()+first_);
        first_ = (first_+1) % max_;
        --count_;
    }
    void pop_back() noexcept {
        alloc_traits::destroy(alloc_(), data_() + (first_+count_-1) % max_);
        --count_;
    }

    const auto& operator[](size_type i) const noexcept {
        return data_()[(first_+i) % max_];
    }
    auto& operator[](size_type i) noexcept {
        return data_()[(first_+i) % max_];
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
            alloc_traits::destroy(alloc_(), &data_()[i % max_]);
            --count_;
        }
    }

private:
    CompressPair<allocator_type, T*> alloc_and_data_;
    auto& alloc_() noexcept { return alloc_and_data_.first(); }
    auto data_() const noexcept { return alloc_and_data_.second(); }
    size_type first_;
    size_type count_;
    size_type max_;
};

} // namespace clst

#endif // CLST_RING_BUFFER_HPP
