#include "crawler/http.hpp"
#include "crawler/walk.hpp"
#include "crawler/blocking_queue.hpp"

#include <filesystem>
#include <exception>
#include <string>
#include <vector>
#include <thread>
#include <functional>//std::ref

namespace {

//队列的数据结构
struct DownloadJob{
    std::filesystem::path dest;
    std::string url;
};

//单次下载
void Download_job(const DownloadJob& job){
    crawler::log_line("downloading " + job.url);
    crawler::http_download(job.url, job.dest);
    crawler::log_line(job.url + " downloaded");
}

//把queue的任务下载完
void worker_loop(crawler::BlockingQueue<DownloadJob>& queue){
    for(;;){
        auto job = queue.pop();
        if(!job){
            break;
        }
        try{
            Download_job(*job);
        }catch(const std::exception& ex){
            crawler::log_line("error downloading " + job->url + ": " + ex.what());
        }
    }
}


void crawling(const std::string& url){
    /*
    TODO
    创建队列，线程，预分配
    遍历唤醒线程emplace_back(要调用的函数，要传入的参数)
    walk函数，找所有要下载的file_url
    queue.close()
    等待线程结束
    */
    crawler::BlockingQueue<DownloadJob> queue;
    std::vector<std::thread> workers;
    workers.reserve(crawler::kWorkerCount);
    for(int i = 0; i < crawler::kWorkerCount; ++i){
        workers.emplace_back(worker_loop,std::ref(queue));
    }

    crawler::walk(url, [&](const std::filesystem::path& dest, const std::string& file_url){
        queue.push({dest,file_url});
    });

    queue.close();

    for(std::thread& worker : workers){
        worker.join();
    }
}
}//匿名命名空间
int main(int argc, char* argv[]) {
    try{
        /*
        TODO
        curl网络初始化
        创建url
        爬虫下载到本地crawling
        */
        crawler::CurlGlobal curl;
        std::string url = argc > 1 ? argv[1] : std::string(crawler::kDefaultUrl);
        crawling(url);
        return 0;
    }catch(std::exception& ex){
        crawler::log_line(std::string("fatal: ") + ex.what());
        return 1;
    }
}