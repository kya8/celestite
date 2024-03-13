#include "clst/concurrency/tpool.h"
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "exceptions.h"

namespace clst::concurrency {

struct ThreadPool::Impl {
    std::vector<std::thread> workers;
    std::deque<std::packaged_task<ReturnType()>> tasks;
    std::size_t max_jobs;

    std::mutex mutex_;
    std::condition_variable cond;
    std::condition_variable cond_enqueue;
    bool stop = false;

    std::size_t nb_working = 0;
    std::condition_variable working_cond;

    Impl(std::size_t N, std::size_t max_jobs_) noexcept : max_jobs(max_jobs_)
    {
        for (std::size_t i = 0; i < N; ++i) {
            workers.emplace_back(
                [this] {
                    for (;;) {
                        TaskType task;

                        {
                            std::unique_lock lk(mutex_);
                            cond.wait(lk,
                                      [&] { return stop || !tasks.empty(); });
                            if (stop && tasks.empty())
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
                });
        }
    }

    void stopAll() noexcept
    {
        {
            std::scoped_lock lk(mutex_);
            stop = true;
        }
        cond.notify_all();
    }

    ~Impl() noexcept
    {
        stopAll();
        for (auto& worker : workers) {
            worker.join();
        }
    }

    void waitAll() noexcept
    {
        std::unique_lock lk(mutex_);
        working_cond.wait(lk, [&] { return nb_working == 0 && tasks.empty(); });
    }

    void enqueue(TaskType&& task)
    {
        {
            std::unique_lock lk(mutex_);
            if (stop) {
                throw error::ThreadPoolError("Cannot enqueue on stopped thread pool.");
            }
            if (max_jobs) {
                cond_enqueue.wait(lk, [&]{return tasks.size() < max_jobs;});
            }
            tasks.emplace_back(std::move(task));
        }
        cond.notify_one();
    }
};

void
ThreadPool::enqueue(TaskType&& task)
{
    impl->enqueue(std::move(task));
}

ThreadPool::ThreadPool(std::size_t nb_threads, std::size_t max_jobs) noexcept : impl(std::make_unique<Impl>(nb_threads, max_jobs)) {}

ThreadPool::~ThreadPool() = default;

void
ThreadPool::stop() noexcept
{
    impl->stopAll();
}

void
ThreadPool::waitAll() noexcept
{
    impl->waitAll();
}

// Ideally, shared locking should be used in observers here... Or use atomics
std::size_t
ThreadPool::getThreadCount() const noexcept
{
    {
        std::scoped_lock lk(impl->mutex_);
        return impl->workers.size();
    }
}
std::size_t
ThreadPool::getQueuedCount() const noexcept
{
    {
        std::scoped_lock lk(impl->mutex_);
        return impl->tasks.size();
    }
}
std::size_t
ThreadPool::getWorkingCount() const noexcept
{
    {
        std::scoped_lock lk(impl->mutex_);
        return impl->nb_working;
    }
}

ThreadPool&
ThreadPool::getDefaultPool(std::size_t N) noexcept
{
    static ThreadPool tp(N);
    return tp;
}

} // namespace clst::concurrency
