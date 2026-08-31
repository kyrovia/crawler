#include "crawler/blocking_queue.hpp"
#include "crawler/http.hpp"
#include "crawler/walk.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

struct DownloadJob {
    std::filesystem::path dest;
    std::string url;
};

void worker_loop(crawler::BlockingQueue<DownloadJob>& queue) {
    for (;;) {
        auto job = queue.pop();
        if (!job) {
            break;
        }
        try {
            crawler::http_download(job->url, job->dest);
        } catch (const std::exception& ex) {
            crawler::log_line(std::string("error downloading ") + job->url + ": " +
                              ex.what());
        }
    }
}

void crawl_serial(const std::string& url) {
    crawler::walk(url, [&](const std::filesystem::path& dest, const std::string& file_url) {
        try {
            crawler::http_download(file_url, dest);
        } catch (const std::exception& ex) {
            crawler::log_line(std::string("error downloading ") + file_url + ": " +
                              ex.what());
        }
    });
}

void crawl_parallel(const std::string& url, int worker_count) {
    crawler::BlockingQueue<DownloadJob> queue;
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(worker_count));
    for (int i = 0; i < worker_count; ++i) {
        workers.emplace_back(worker_loop, std::ref(queue));
    }

    crawler::walk(url, [&](const std::filesystem::path& dest, const std::string& file_url) {
        queue.push({dest, file_url});
    });

    queue.close();
    for (std::thread& worker : workers) {
        worker.join();
    }
}

void clean_dest(const std::string& url) {
    const std::filesystem::path dest = crawler::local_path(url);
    if (dest == std::filesystem::path(".")) {
        return;
    }
    std::filesystem::remove_all(dest);
}

template <typename Fn>
long long time_ms(Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    fn();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

std::string test_url() {
    const char* env = std::getenv("CRAWLER_TEST_URL");
    if (env != nullptr && env[0] != '\0') {
        return env;
    }
    return std::string(crawler::kDefaultUrl);
}

}  // namespace

class CompareSpeedTest : public ::testing::TestWithParam<int> {
protected:
    static void SetUpTestSuite() { curl_ = new crawler::CurlGlobal(); }

    static void TearDownTestSuite() {
        delete curl_;
        curl_ = nullptr;
    }

    static crawler::CurlGlobal* curl_;
};

crawler::CurlGlobal* CompareSpeedTest::curl_ = nullptr;

// GetParam() == 0 表示单线程；其余为工作线程数。
TEST_P(CompareSpeedTest, ReportsWallTime) {
    const std::string url = test_url();
    const int workers = GetParam();

    clean_dest(url);

    long long ms = 0;
    ASSERT_NO_THROW({
        if (workers == 0) {
            ms = time_ms([&] { crawl_serial(url); });
        } else {
            ms = time_ms([&] { crawl_parallel(url, workers); });
        }
    });

    if (workers == 0) {
        std::cout << "serial: " << ms << " ms\n";
    } else {
        std::cout << workers << " threads: " << ms << " ms\n";
    }
    RecordProperty("wall_ms", static_cast<int>(ms));
    EXPECT_GT(ms, 0);
}

INSTANTIATE_TEST_SUITE_P(WorkerCount, CompareSpeedTest,
                         ::testing::Values(0, 12, 10, 8, 6, 4, 2),
                         [](const ::testing::TestParamInfo<int>& info) {
                             if (info.param == 0) {
                                 return std::string("Serial");
                             }
                             return "Threads" + std::to_string(info.param);
                         });
