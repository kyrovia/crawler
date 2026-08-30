#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace crawler {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count) {
        if (worker_count == 0) {
            throw std::invalid_argument("thread pool needs at least one worker");
        }
        workers_.reserve(worker_count);
        for (std::size_t i = 0; i < worker_count; ++i) {
            workers_.emplace_back([this] { run(); });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template <typename F>
    void submit(F&& job) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stop_) {
                throw std::runtime_error("submit on stopped thread pool");
            }
            jobs_.emplace(std::forward<F>(job));
        }
        cv_.notify_one();
    }

private:
    void run() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !jobs_.empty(); });
                if (stop_ && jobs_.empty()) {
                    return;
                }
                job = std::move(jobs_.front());
                jobs_.pop();
            }
            job();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> jobs_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

}  // namespace crawler
