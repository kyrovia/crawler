#pragma once

#include "crawler/html.hpp"
#include "crawler/http.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace crawler {

inline constexpr std::string_view kPathMarker = "gnu/bool/";
inline constexpr std::string_view kDefaultUrl = "https://ftp.gnu.org/gnu/bool/";
inline constexpr int kWorkerCount = 8;

bool is_directory_url(std::string_view url);
bool should_follow(std::string_view href);
std::filesystem::path local_path(std::string_view url);
void log_line(std::string_view message);

// DFS 遍历目录页；遇到文件时调用 on_file(本地路径, URL)。
template <typename OnFile>
void walk(std::string start_url, OnFile&& on_file) {
    std::vector<std::string> stack;
    stack.push_back(std::move(start_url));

    while (!stack.empty()) {
        std::string top = std::move(stack.back());
        stack.pop_back();

        const auto fdir = local_path(top);
        if (is_directory_url(top)) {
            log_line(top);
            std::filesystem::create_directories(fdir);
            const std::string html = http_get(top);
            for (const std::string& href : extract_hrefs(html)) {
                if (should_follow(href)) {
                    stack.push_back(top + href);
                }
            }
        } else {
            on_file(fdir, top);
        }
    }
}

}  // namespace crawler
