#ifndef CLST_CONCURRENCY_CHANNEL_H
#define CLST_CONCURRENCY_CHANNEL_H

#include <deque>
#include <mutex>
#include <condition_variable>

namespace clst::concurrency {

/**
 * A simple thread-safe queue.
 * A Channel can be closed, after which enqueuing will return false.
 * Remaining items in a closed Channel can still be retrived, until it's empty.
 */
template<class T>
class Channel : protected std::deque<T> {
public:
    using container_type = std::deque<T>;
    using typename container_type::value_type;
    using typename container_type::size_type;
    using typename container_type::reference;
    using typename container_type::const_reference;
private:
    std::mutex mtx_;
    std::condition_variable cond_emplace_;
    std::condition_variable cond_pop_;
    const size_type max_ = 0;
    bool closed_ = false;
public:
    Channel(size_type max_items) : max_(max_items) {};

    // copy / move...

    // methods that return references are disallowed!

    bool empty();
    size_type size();

    template<typename ...Ts>
    bool emplace(Ts ...Args);
    bool pop(value_type& dst); // pop to destination

    void close();

    void clear();

    bool operator>>(value_type &rhs)
    {
        return pop(rhs);
    }
    bool operator<<(const value_type &rhs)
    {
        return emplace(rhs);
    }
    bool operator<<(value_type &&rhs)
    {
        return emplace(std::move(rhs));
    }
};

template<class T>
inline bool
Channel<T>::empty()
{
    std::scoped_lock lk(mtx_);
    return container_type::empty();
}

template<class T>
inline typename Channel<T>::size_type
Channel<T>::size()
{
    std::scoped_lock lk(mtx_);
    return container_type::size();
}

template<class T>
template<typename ...Ts>
inline bool
Channel<T>::emplace(Ts ...Args)
{
    {
        std::unique_lock lk(mtx_);
        if (max_) {
            cond_emplace_.wait(lk, [&] { return container_type::size() < max_ || closed_; });
        }
        if (closed_) return false;
        container_type::emplace_back(std::forward<Ts>(Args)...);
    }
    cond_pop_.notify_one();

    return true;
}

template<class T>
inline bool
Channel<T>::pop(value_type& dst)
{
    {
        std::unique_lock lk(mtx_);
        cond_pop_.wait(lk, [&] { return closed_ || !container_type::empty(); });
        if (closed_ && container_type::empty()) {
            return false;
        }
        dst = std::move(container_type::front());
        container_type::pop_front();
    }
    if (max_) {
        cond_emplace_.notify_one();
    }

    return true;
}

template<class T>
inline void
Channel<T>::close()
{
    {
        std::scoped_lock lk(mtx_);
        closed_ = true;
    }
    cond_pop_.notify_all();
    if (max_) cond_emplace_.notify_all();
}

template<class T>
inline void
Channel<T>::clear()
{
    {
        std::scoped_lock lk(mtx_);
        container_type::clear();
    }
    if (max_) cond_emplace_.notify_all();
}

} // namespace clst::concurrency


#endif /* CLST_CONCURRENCY_CHANNEL_H */
