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
    void stopAll() noexcept;
private:
    std::vector<std::thread> workers;
    std::deque<std::function<void()>> tasks;
    std::size_t max_jobs;

    std::mutex mutex;
    std::condition_variable cond;
    std::condition_variable cond_enqueue;
    bool stop = false;

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
                        cond.wait(lk, [&]{ return stop || !tasks.empty(); });
                        if(stop && tasks.empty())
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
        if(stop) {
            throw error::ThreadPoolError("Cannot enqueue on stopped thread pool.");
        }
        if (max_jobs) {
            cond_enqueue.wait(lk, [&] { return tasks.size() < max_jobs; });
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
        if(stop) {
            throw error::ThreadPoolError("Cannot enqueue on stopped thread pool.");
        }
        if (max_jobs) {
            cond_enqueue.wait(lk, [&] { return tasks.size() < max_jobs; });
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
    stopAll();
}

// "graceful" stop
// This'll prevent adding new task, finish the remaining task queue, and join all worker threads.
inline void SimpleThreadPool::stopAll() noexcept
{
    {
        std::scoped_lock lk(mutex);
        stop = true;
    }
    cond.notify_all();

    for (auto& worker : workers)
        worker.join();
}

// Returns when there is no thread working, and no task queued.
inline void SimpleThreadPool::waitAll() noexcept
{
    std::unique_lock lk(mutex);
    cond_pool.wait(lk, [&] { return nb_working == 0 && tasks.empty(); });
}

} // namespace clst::concurrency


#endif /* CLST_CONCURRENCY_SIMPLE_THREAD_POOL_H */
