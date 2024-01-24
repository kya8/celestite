#include "clst/concurrency/tpool.h"
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace clst::concurrency {

struct tpool::Impl {
    std::vector<std::thread> workers;
    std::deque<std::packaged_task<ReturnType()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable cond;
    bool stop = false;

    std::size_t nb_working = 0;
    std::condition_variable working_cond;

    Impl(std::size_t N) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) {
            workers.emplace_back(
                [this] {
                    for (;;) {
                        TaskType task;

                        {
                            std::unique_lock lk(queue_mutex);
                            cond.wait(lk,
                                      [&] { return stop || !tasks.empty(); });
                            if (stop && tasks.empty())
                                return;
                            task = std::move(tasks.front());
                            tasks.pop_front();
                            nb_working += 1;
                        }

                        task();

                        {
                            std::scoped_lock lk(queue_mutex);
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
            std::scoped_lock lk(queue_mutex);
            stop = true;
        }
        cond.notify_all();
        for (auto& worker : workers)
            worker.join();
    }

    ~Impl() noexcept
    {
        stopAll();
    }

    void waitAll() noexcept
    {
        std::unique_lock lk(queue_mutex);
        working_cond.wait(lk, [&] { return nb_working == 0 && tasks.empty(); });
    }

    void enqueue(TaskType&& task)
    {
        {
            std::scoped_lock lk(queue_mutex);
            if (stop) {
                throw std::runtime_error("Enqueuing on stopped thread pool");
            }

            tasks.emplace_back(std::move(task));
        }
        cond.notify_one();
    }
};

void
tpool::enqueue(TaskType&& task)
{
    impl->enqueue(std::move(task));
}

tpool::tpool(std::size_t nb_threads) noexcept : impl(std::make_unique<Impl>(nb_threads))
{}

tpool::~tpool() = default;

void
tpool::stopAll() noexcept
{
    impl->stopAll();
}

void
tpool::waitAll() noexcept
{
    impl->waitAll();
}

std::size_t
tpool::getThreadsNum() const noexcept
{
    return impl->workers.size();
}


tpool&
tpool::getDefaultPool(std::size_t N) noexcept
{
    static tpool tp(N);
    return tp;
}

} // namespace clst::concurrency
