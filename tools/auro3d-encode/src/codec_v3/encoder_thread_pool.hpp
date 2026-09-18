#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace auro3d::encode {

/// Fixed worker pool shared by every UnitBlock in one encoder run. The caller
/// thread remains available as an additional worker for deterministic group
/// analysis, so worker_count=N-1 implements a user-facing total of N threads.
class EncoderThreadPool {
public:
    explicit EncoderThreadPool(std::size_t worker_count) {
        workers_.reserve(worker_count);
        for (std::size_t index = 0u; index < worker_count; ++index) {
            workers_.emplace_back([this]() { worker_loop(); });
        }
    }

    ~EncoderThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        ready_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable())
                worker.join();
        }
    }

    EncoderThreadPool(const EncoderThreadPool&) = delete;
    EncoderThreadPool& operator=(const EncoderThreadPool&) = delete;

    std::size_t worker_count() const {
        return workers_.size();
    }

    template <typename Function>
    auto submit(Function&& function)
        -> std::future<std::invoke_result_t<std::decay_t<Function>>> {
        using Result = std::invoke_result_t<std::decay_t<Function>>;
        auto task = std::make_shared<std::packaged_task<Result()>>(
            std::forward<Function>(function));
        std::future<Result> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_)
                throw std::runtime_error("encoder thread pool is stopping");
            tasks_.emplace([task]() { (*task)(); });
        }
        ready_.notify_one();
        return result;
    }

private:
    void worker_loop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                ready_.wait(lock, [this]() {
                    return stopping_ || !tasks_.empty();
                });
                if (stopping_ && tasks_.empty())
                    return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable ready_;
    bool stopping_ = false;
};

} // namespace auro3d:encode
