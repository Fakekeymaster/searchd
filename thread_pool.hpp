#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

// A simple thread pool.
// How it works:
//   - We spawn N worker threads at startup, they all block on a condition_variable.
//   - submit() wraps your callable into a packaged_task, pushes it onto the queue,
//     and notifies one sleeping worker.
//   - The worker wakes up, pops the task, runs it, and goes back to sleep.
//   - submit() returns a std::future so the caller can collect the result later.

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) : stop_(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        // Sleep until there's a task or we're shutting down
                        cv_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        if (stop_ && tasks_.empty()) return;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task(); // run outside the lock
                }
            });
        }
    }

    // Submit any callable, get back a future for its return value
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using ReturnType = typename std::result_of<F(Args...)>::type;

        // packaged_task binds the callable and makes it future-compatible
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) throw std::runtime_error("ThreadPool is stopped");
            tasks_.emplace([task] { (*task)(); });
        }
        cv_.notify_one(); // wake one worker
        return result;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        cv_.notify_all(); // wake everyone so they can exit
        for (auto& w : workers_) w.join();
    }

private:
    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        queue_mutex_;
    std::condition_variable           cv_;
    bool                              stop_;
};
