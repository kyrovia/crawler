#include "crawler/blocking_queue.hpp"
#include "crawler/http.hpp"
#include "crawler/walk.hpp"

#include <exception>
#include <filesystem>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct DownloadJob {
    std::filesystem::path dest;
    std::string url;
};

void download_job(const DownloadJob& job) {
    crawler::log_line("downloading " + job.url);
    crawler::http_download(job.url, job.dest);
    crawler::log_line(job.url + " downloaded");
}

void worker_loop(crawler::BlockingQueue<DownloadJob>& queue) {
    for (;;) {
        auto job = queue.pop();
        if (!job) {
            break;
        }
        try {
            download_job(*job);
        } catch (const std::exception& ex) {
            crawler::log_line(std::string("download failed: ") + ex.what());
        }
    }
}

void crawling(const std::string& url) {
    crawler::BlockingQueue<DownloadJob> queue;
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(crawler::kWorkerCount));
    for (int i = 0; i < crawler::kWorkerCount; ++i) {
        workers.emplace_back(worker_loop, std::ref(queue));
    }

    crawler::walk(url, [&](const std::filesystem::path& dest, const std::string& file_url) {
        queue.push(DownloadJob{dest, file_url});
    });

    queue.close();
    for (std::thread& worker : workers) {
        worker.join();
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        crawler::CurlGlobal curl;
        const std::string url = argc > 1 ? argv[1] : std::string(crawler::kDefaultUrl);
        crawling(url);
        return 0;
    } catch (const std::exception& ex) {
        crawler::log_line(std::string("fatal: ") + ex.what());
        return 1;
    }
}
