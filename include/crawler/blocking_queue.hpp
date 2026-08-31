#pragma once

#include <queue>
#include <optional>
#include <utility>//std::move
#include <mutex>
#include <condition_variable>

namespace crawler {

/*
queue为共享数据，需要使用线程锁和condition_variable保证线程安全和效率
*/
template<typename T>
class BlockingQueue{
public:

    void push(T value){
        std::lock_guard<std::mutex> lock(mutex_);
        if(closed_){
            return;
        }
        queue_.push(std::move(value));//避免复制更高效
        cv_.notify_one();//唤醒一个consumer的线程
    }

    std::optional<T> pop(){
        /*
        TODO
        unique_lock锁
        cv_.wait
        判断是否队列为空且未关闭
        取queue.front
        return
        */
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock,[this]{return closed_|| !queue_.empty();});

        if(queue_.empty()){
            return std::nullopt;
        }//队列为空且关闭

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    //没有新任务时触发
    void close(){
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }//作用域块，锁在作用域结束后自动析构
        cv_.notify_all();//唤醒所有consumer的线程
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool closed_ = false;
};
}
