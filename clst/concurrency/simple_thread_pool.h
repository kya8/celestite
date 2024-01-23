#ifndef CLST_CONCURRENCY_SIMPLE_THREAD_POOL_H
#define CLST_CONCURRENCY_SIMPLE_THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <cstddef>
#include <type_traits>

namespace clst::concurrency {

class simple_thread_pool {
public:
    simple_thread_pool(std::size_t nb_threads);
    ~simple_thread_pool();

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args);

    template<typename F, typename... Args>
    void enqueue_without_future(F&& f, Args&&... args);

    void wait_all();
    void stop_all();
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex mutex;
    std::condition_variable cond;
    bool stop = false;

    std::size_t nb_working = 0;
    std::condition_variable cond_pool;
};


inline simple_thread_pool::simple_thread_pool(std::size_t threads)
{
    for(std::size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock lk(this->mutex);
                        this->cond.wait(lk,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                        nb_working += 1;
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
auto simple_thread_pool::enqueue(F&& f, Args&&... args)
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
        std::scoped_lock lk(mutex);
        if(stop) {
            throw std::runtime_error("Enqueuing on stopped thread pool");
        }
        tasks.emplace([task = std::move(task)]()->void { (*task)(); });
    }
    cond.notify_one();
    return future;
}

template<typename F, typename... Args>
void simple_thread_pool::enqueue_without_future(F&& f, Args&&... args)
{
    {
        std::scoped_lock lk(mutex);
        if(stop) {
            throw std::runtime_error("Enqueuing on stopped thread pool");
        }
        if constexpr (sizeof...(Args) == 0) {
            tasks.emplace(std::forward<F>(f));
        }
        else {
            tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }
    }
    cond.notify_one();
}


inline simple_thread_pool::~simple_thread_pool()
{
    stop_all();
}

// "graceful" stop
// This'll prevent adding new task, finish the remaining task queue, and join all worker threads.
inline void simple_thread_pool::stop_all()
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
inline void simple_thread_pool::wait_all()
{
    std::unique_lock lk(mutex);
    cond_pool.wait(lk, [&] { return nb_working == 0 && tasks.empty(); });
}

} // namespace clst::concurrency


#endif /* CLST_CONCURRENCY_SIMPLE_THREAD_POOL_H */
