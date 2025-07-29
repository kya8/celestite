#ifndef CLST_THREAD_POOL_HPP
#define CLST_THREAD_POOL_HPP

#include <future>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "clst/error.hpp"

/**
 * TODO:
 *
 * - Currently, std::packaged_task is used for type-erased task storage, and per-task future management.
 *   It has quite some overhead if futures aren't required.
 *   Ideally, we want to use std::move_only_function here.
 *   Also, provide a template parameter to select between task storage at compile time.
 *
 * - Use a template parameter to specialize unbounded / bounded task queue.
 *   A ring buffer should be used for the bounded case.
 *   Currently we always use a dequeue, with optional runtime limit on its size.
 */

namespace clst {

template<typename R = void>
class ThreadPool {
public:
    using ReturnType = R;

    ThreadPool(std::size_t nb_threads, std::size_t max_jobs = 0) noexcept;

    /**
     * Initialize workers with a custom callable.
     */
    template<typename F>
    ThreadPool(const F& init_fn, std::size_t nb_threads, std::size_t max_jobs) noexcept;

    ~ThreadPool() noexcept;

    // Not copiable.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Not movable.
    // If you really need to move me, wrap me inside unique_ptr.

    /**
     * Request to stop the pool.
     *
     * Remaining tasks will continue on. Enqueuing is blocked.
     */
    void stop() noexcept;

    /**
     * Request to stop the pool immediately. Discards tasks in the queue.
     */
    void stop_now() noexcept;

    /**
     * Wait for all tasks to finish.
     */
    void wait_all() noexcept;

    /**
     * Enqueue task and receive a future.
     */
    template<typename F>
    [[nodiscard]] auto submit(F&& f);

    /**
     * Enqueue task without getting a future.
     */
    template<typename F>
    void enqueue(F&& f);

    /* Exception throwed when enqueuing on a stopping or stopped pool */
    class EnqueueBlocked : public Error {
    public:
        const char* what() const noexcept override
        {
            return "Cannot enqueue on stopped thread pool.";
        }
    };


private:
    std::vector<std::thread> workers;

    std::mutex mutex_;
    std::condition_variable cond;
    bool stop_ = false;

    std::size_t nb_working = 0;
    std::condition_variable working_cond;

    std::condition_variable cond_enqueue;
    std::size_t max_jobs;

    using TaskT = std::packaged_task<R()>;
    std::deque<TaskT> tasks;

    void enqueue_task(TaskT&& task);

    void worker_loop()
    {
        for (;;) {
            TaskT task;

            {
                std::unique_lock lk(mutex_);
                cond.wait(lk, [&] { return stop_ || !tasks.empty(); });
                if (tasks.empty())
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
                std::scoped_lock lk(mutex_);
                if (--nb_working == 0) {
                    working_cond.notify_all();
                }
            }
        }
    }
};

template<typename R>
inline ThreadPool<R>::ThreadPool(std::size_t N, std::size_t max_jobs_) noexcept : max_jobs(max_jobs_)
{
    for (std::size_t i = 0; i < N; ++i) {
        workers.emplace_back( [this] {
            worker_loop();
        });
    }
}

template<typename R>
template<typename F>
inline ThreadPool<R>::ThreadPool(const F& init_fn, std::size_t N, std::size_t max_jobs_) noexcept : max_jobs(max_jobs_)
{
    for (std::size_t i = 0; i < N; ++i) {
        workers.emplace_back( [this, init_fn] { // init_fn must be copyable
            init_fn();
            worker_loop();
        });
    }
}

template<typename R>
inline void ThreadPool<R>::stop() noexcept
{
    {
        std::scoped_lock lk(mutex_);
        stop_ = true;
    }
    cond.notify_all();
    if (max_jobs) cond_enqueue.notify_all();
}

template<typename R>
inline void ThreadPool<R>::stop_now() noexcept
{
    {
        std::scoped_lock lk(mutex_);
        stop_ = true;
        tasks.clear();
        if (nb_working == 0) {
            working_cond.notify_all();
        }
    }
    cond.notify_all();
    if (max_jobs) cond_enqueue.notify_all();
}

template<typename R>
inline ThreadPool<R>::~ThreadPool() noexcept
{
    stop();
    for (auto& worker : workers) {
        worker.join();
    }
}

template<typename R>
inline void ThreadPool<R>::wait_all() noexcept
{
    std::unique_lock lk(mutex_);
    working_cond.wait(lk, [&] { return nb_working == 0 && tasks.empty(); });
}

template<typename R>
template<typename F>
inline void ThreadPool<R>::enqueue(F&& f)
{
    enqueue_task(TaskT(std::forward<F>(f)));
}

template<typename R>
template<typename F>
inline auto ThreadPool<R>::submit(F&& f)
{
    auto task = TaskT(std::forward<F>(f));
    auto ret  = task.get_future();
    enqueue_task(std::move(task));
    return ret;
}

template<typename R>
inline void ThreadPool<R>::enqueue_task(TaskT&& task)
{
    {
        std::unique_lock lk(mutex_);
        if (max_jobs) {
            cond_enqueue.wait(lk, [&] { return (tasks.size() < max_jobs) || stop_; });
        }
        if (stop_) throw EnqueueBlocked{};
        tasks.emplace_back(std::move(task));
    }
    cond.notify_one();
}

} // namespace clst

#endif // CLST_THREAD_POOL_HPP
