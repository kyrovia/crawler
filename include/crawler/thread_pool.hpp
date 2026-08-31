#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <vector>
#include <functional>//函数包装器
#include <stdexcept>

namespace crawler {
class ThreadPool{
public:
    /*
    构造函数
    析构函数
    submit()
    */
    //创建线程，并启动线程
    explicit ThreadPool(std::size_t worker_count){
        if(worker_count == 0){
            throw std::invalid_argument("thread pool needs at least one thread");
        }
        workers_.reserve(worker_count);

        for(std::size_t i = 0; i < worker_count; i++){
            workers_.emplace_back([this]{run();});
        }
    }

    //禁止拷贝
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    //标记关闭并等待所有线程结束
    ~ThreadPool(){
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }//stop_属于共享变量，需要加锁
        cv_.notify_all();

        for(std::thread& worker : workers_){
            if(worker.joinable()){
                worker.join();
            }
        }
    }

    //将job传入jobs并唤醒线程
    template<typename F>
    void submit(F&& job){
        { 
            std::lock_guard<std::mutex> lock(mutex_);
            if(stop_){
                throw std::runtime_error("thread pool is stopped");
            }    
            jobs_.emplace(std::forward<F>(job));//emplace 直接在队列里创建job并传入
        }
        cv_.notify_one();
    }

private:
    /*
    TODO
    void run()
    workers,jobs,cv,stop
    */
    void run(){
        for(;;){
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock,[this]{return stop_ || !jobs_.empty();});
                if(stop_ && jobs_.empty()){
                    return;
                }
                job = std::move(jobs_.front());
                jobs_.pop();
            }//作用域块，结束自动析构解锁，把取任务和执行任务的线程分离
            job();//执行任务
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> jobs_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};
}