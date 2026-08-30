#include "crawler/http.hpp"
#include "crawler/thread_pool.hpp"
#include "crawler/walk.hpp"

#include <exception>
#include <filesystem>
#include <string>

namespace {

void download_file(const std::filesystem::path& dest, const std::string& url) {
    crawler::log_line("downloading " + url);
    crawler::http_download(url, dest);
    crawler::log_line(url + " downloaded");
}

void crawling(const std::string& url) {
    crawler::ThreadPool pool(static_cast<std::size_t>(crawler::kWorkerCount));
    crawler::walk(url, [&](const std::filesystem::path& dest, const std::string& file_url) {
        pool.submit([dest, file_url] {
            try {
                download_file(dest, file_url);
            } catch (const std::exception& ex) {
                crawler::log_line(std::string("download failed: ") + ex.what());
            }
        });
    });
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
