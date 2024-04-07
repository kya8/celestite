#ifndef CLST_CONCURRENCY_TPOOL_H
#define CLST_CONCURRENCY_TPOOL_H

#include <future>
#include <memory>
#include <cstddef>
#include <string_view>

namespace clst::concurrency {

class ThreadPool {
public:
    ThreadPool(std::size_t nb_threads, std::size_t max_jobs = 0) noexcept;
    ~ThreadPool() noexcept;

    void stop() noexcept;     // request stop
    void stopNow() noexcept;  // also discard queued jobs
    void waitAll() noexcept;
    std::size_t getThreadCount() const noexcept;
    std::size_t getQueuedCount() const noexcept;
    std::size_t getWorkingCount() const noexcept;

    using ReturnType = int;

    template<typename F>
    auto enqueueFuture(F&& f) {
        auto task = TaskType(std::forward<F>(f));
        auto ret = task.get_future();
        enqueue(std::move(task));
        return ret;
    }

    template<typename F>
    void enqueue(F&& f) {
        enqueue(TaskType(std::forward<F>(f)));
    }

    /**
     * Get a thread pool from a global registry, by name.
     * 
     * The thread pool will be constructed with given arguments if it does not exist.
     */
    static ThreadPool& getGlobalPool(std::string_view name, std::size_t threads = 1, std::size_t max_jobs = 0) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    using TaskType = std::packaged_task<ReturnType()>;
    void enqueue(TaskType&& task);
};

} // namespace clst::concurrency

#endif /* CLST_CONCURRENCY_TPOOL_H */
