#ifndef CLST_CONCURRENCY_TP_H
#define CLST_CONCURRENCY_TP_H

#include <future>
#include <memory>
#include <cstddef>

namespace clst::concurrency {

class tpool {
public:
    tpool(std::size_t nb_threads) noexcept;
    ~tpool() noexcept;

    void stopAll() noexcept;
    void waitAll() noexcept;
    std::size_t getThreadsNum() const noexcept;

    using ReturnType = long long int;

    template<typename F>
    auto enqueue(F&& f) {
        auto task = TaskType(std::forward<F>(f));
        auto ret = task.get_future();
        enqueue(std::move(task));
        return ret;
    }

    static tpool& getDefaultPool(std::size_t N=0) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    using TaskType = std::packaged_task<ReturnType()>;
    void enqueue(TaskType&& task);
};

} // namespace clst::concurrency

#endif /* CLST_CONCURRENCY_TP_H */