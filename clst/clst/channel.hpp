#ifndef CLST_CHANNEL_HPP
#define CLST_CHANNEL_HPP

#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <cassert>
#include <type_traits>

namespace clst {

namespace detail {

template<bool Bounded>
struct ChannelSize {};

template<>
struct ChannelSize<true> {
    std::size_t max_;
    std::condition_variable cond_emplace_;

    ChannelSize(std::size_t max) : max_(max) {}
};

}

//FIXME: Use a ring buffer when bounded.

template<class T, bool Bounded = true, bool Sp = false, bool Sc = false>
class Channel : protected std::deque<T>, protected detail::ChannelSize<Bounded> {
public:
    using Container = std::deque<T>;
    using typename Container::value_type;
    using typename Container::size_type;
    using typename Container::reference;
    using typename Container::const_reference;

    static constexpr bool is_bounded = Bounded;
    static constexpr bool is_single_producer = Sp;
    static constexpr bool is_single_consumer = Sc;
private:
    std::mutex mtx_;
    std::condition_variable cond_pop_;
    bool closed_ = false;
public:
    template<bool B = Bounded, typename = std::enable_if_t<B>> // Unnecessary?
    Channel(size_type max_items) : detail::ChannelSize<Bounded>{(assert(max_items > 0), max_items)} {};

    Channel() = default; // default = Implicitly deleted, if Bounded.

    // Methods that return references are disallowed!

    bool empty()
    {
        std::lock_guard lk(mtx_);
        return Container::empty();
    }
    size_type size()
    {
        std::lock_guard lk(mtx_);
        return Container::size();
    }

    template<typename ...Ts>
    bool emplace(Ts&& ...Args)
    {
        if constexpr (Sc) {
            bool should_notify; // For single-consumer, we only need to notify the consumer when filling an empty queue.
            {
                std::unique_lock lk(mtx_);
                if constexpr (Bounded) {
                    this->cond_emplace_.wait(lk, [&] { return Container::size() < this->max_ || closed_; });
                }
                if (closed_) {
                    return false;
                }
                should_notify = Container::empty();
                Container::emplace_back(std::forward<Ts>(Args)...);
            }
            if (should_notify) {
                cond_pop_.notify_one();
            }
            return true;
        } else {
            {
                std::unique_lock lk(mtx_);
                if constexpr (Bounded) {
                    this->cond_emplace_.wait(lk, [&] { return Container::size() < this->max_ || closed_; });
                }
                if (closed_) {
                    return false;
                }
                Container::emplace_back(std::forward<Ts>(Args)...);
            }
            cond_pop_.notify_one();
            return true;
        }
    }

    void close()
    {
        {
            std::lock_guard lk(mtx_);
            closed_ = true;
        }
        cond_pop_.notify_all();
        if constexpr (Bounded) {
            this->cond_emplace_.notify_all();
        }
    }

    bool pop(value_type& dst) // pop to destination
    {
        if constexpr (!Bounded || !Sp) {
            {
                std::unique_lock lk(mtx_);
                cond_pop_.wait(lk, [&] { return closed_ || !Container::empty(); });
                if (Container::empty()) { // continue if closed but not empty
                    return false;
                }
                dst = std::move(Container::front());
                Container::pop_front();
            }
            if constexpr (Bounded) {
                this->cond_emplace_.notify_one();
            }
        } else {
            bool should_notify; // For single-producer, we only need to notify the producer when taking from a full queue (Bounded).
            {
                std::unique_lock lk(mtx_);
                cond_pop_.wait(lk, [&] { return closed_ || !Container::empty(); });
                if (Container::empty()) { // continue if closed but not empty
                    return false;
                }
                should_notify = Container::size() == this->max_;
                dst = std::move(Container::front());
                Container::pop_front();
            }
            if (should_notify) {
                this->cond_emplace_.notify_one();
            }
        }

        return true;
    }

    std::optional<value_type> pop()
    {
        std::optional<value_type> ret;

        if constexpr (!Bounded || !Sp) {
            {
                std::unique_lock lk(mtx_);
                cond_pop_.wait(lk, [&] { return closed_ || !Container::empty(); });
                if (Container::empty()) {
                    return ret;
                }
                ret = std::move(Container::front());
                Container::pop_front();
            }
            if constexpr (Bounded) {
                this->cond_emplace_.notify_one();
            }
        } else {
            bool should_notify;
            {
                std::unique_lock lk(mtx_);
                cond_pop_.wait(lk, [&] { return closed_ || !Container::empty(); });
                if (Container::empty()) {
                    return ret;
                }
                should_notify = Container::size() == this->max_;
                ret = std::move(Container::front());
                Container::pop_front();
            }
            if (should_notify) {
                this->cond_emplace_.notify_one();
            }
        }

        return ret;
    }

    void clear()
    {
        {
            std::lock_guard lk(mtx_);
            Container::clear();
        }
        if constexpr (Bounded) {
            this->cond_emplace_.notify_all();
        }
    }
};

// Partial CTAD isn't possible, so we resort to factory functions
// to deduce `Bounded` based on whether a max_size argument is provided.

template<class T, bool Sp = false, bool Sc = false>
inline auto make_channel()
{
    return Channel<T, false, Sp, Sc>{};
}

template<class T, bool Sp = false, bool Sc = false>
inline auto make_channel(typename std::deque<T>::size_type max_size)
{
    return Channel<T, true, Sp, Sc>{max_size};
}

} // namespace clst

#endif // CLST_CHANNEL_HPP
