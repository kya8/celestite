#ifndef CLST_CONCURRENCY_SIMPLE_THREAD_POOL_H
#define CLST_CONCURRENCY_SIMPLE_THREAD_POOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <vector>
#include <deque>
#include <memory>
#include <functional>
#include <cstddef>
#include <type_traits>
#include "exceptions.h"

namespace clst::concurrency {

class SimpleThreadPool {
public:
    SimpleThreadPool(std::size_t nb_threads, std::size_t max_jobs = 0) noexcept;
    ~SimpleThreadPool() noexcept;

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args);

    template<typename F, typename... Args>
    void enqueueWithoutFuture(F&& f, Args&&... args);

    void waitAll() noexcept;
    void stop() noexcept;    // Signals stop. Does not wait.
    void stopNow() noexcept; // also discards jobs in queue
private:
    std::vector<std::thread> workers;
    std::deque<std::function<void()>> tasks;
    std::size_t max_jobs;

    std::mutex mutex;
    std::condition_variable cond;
    std::condition_variable cond_enqueue;
    bool stop_ = false;

    std::size_t nb_working = 0;
    std::condition_variable cond_pool;
};


inline SimpleThreadPool::SimpleThreadPool(std::size_t nb_threads, std::size_t max_jobs_) noexcept : max_jobs(max_jobs_)
{
    for(std::size_t i = 0;i<nb_threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock lk(mutex);
                        cond.wait(lk, [&]{ return stop_ || !tasks.empty(); });
                        if(stop_ && tasks.empty())
                            return;
                        task = std::move(tasks.front());
                        tasks.pop_front();
                        nb_working += 1;
                    }
                    if (max_jobs) {
                        cond_enqueue.notify_one();
                    }

                    task();

                    {
                        std::scoped_lock lk(mutex);
                        if (--nb_working == 0) {
                            cond_pool.notify_all();
                        }
                    }
                }
            }
        );
}


template<typename F, typename... Args>
auto SimpleThreadPool::enqueue(F&& f, Args&&... args)
{
    using return_type = std::invoke_result_t<F, Args...>;

    auto task =
        [&]{
            if constexpr (sizeof...(Args) == 0) {
                return std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
            }
            else {
                // C++20: Use variadic forward capture with lambda
                return std::make_shared<std::packaged_task<return_type()>>(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            }
        }();
    // shared_ptr needed because std::function requires copyable callable type... Until std::move_only_function.

    auto future = task->get_future();

    {
        std::unique_lock lk(mutex);
        if (stop_) {
            throw error::ThreadPoolError("Cannot enqueue on stopped thread pool.");
        }
        if (max_jobs) {
            cond_enqueue.wait(lk, [&] { return (tasks.size() < max_jobs) || stop_; });
            if (stop_) throw error::ThreadPoolError("Thread pool is stopping, enqueue cancelled.");
        }
        tasks.emplace_back([task = std::move(task)]()->void { (*task)(); });
    }
    cond.notify_one();
    return future;
}

template<typename F, typename... Args>
void SimpleThreadPool::enqueueWithoutFuture(F&& f, Args&&... args)
{
    {
        std::unique_lock lk(mutex);
        if (stop_) {
            throw error::ThreadPoolError("Cannot enqueue on stopped thread pool.");
        }
        if (max_jobs) {
            cond_enqueue.wait(lk, [&] { return (tasks.size() < max_jobs) || stop_; });
            if (stop_) throw error::ThreadPoolError("Thread pool is stopping, enqueue cancelled.");
        }
        if constexpr (sizeof...(Args) == 0) {
            tasks.emplace_back(std::forward<F>(f));
        }
        else {
            tasks.emplace_back(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }
    }
    cond.notify_one();
}


inline SimpleThreadPool::~SimpleThreadPool() noexcept
{
    stop();
    for (auto &worker : workers) {
        worker.join();
    }
}

inline void SimpleThreadPool::stop() noexcept
{
    {
        std::scoped_lock lk(mutex);
        stop_ = true;
    }
    cond.notify_all();
    if (max_jobs) cond_enqueue.notify_all();
}

inline void SimpleThreadPool::stopNow() noexcept
{
    {
        std::scoped_lock lk(mutex);
        stop_ = true;
        tasks.clear();
    }
    cond.notify_all();
    if (max_jobs) cond_enqueue.notify_all();
}

// Returns when there is no thread working, and no task queued.
// Does not take into account enqueuers.
inline void SimpleThreadPool::waitAll() noexcept
{
    std::unique_lock lk(mutex);
    cond_pool.wait(lk, [&] { return nb_working == 0 && tasks.empty(); });
}

} // namespace clst::concurrency


#endif /* CLST_CONCURRENCY_SIMPLE_THREAD_POOL_H */
