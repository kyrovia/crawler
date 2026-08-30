#include "crawler/walk.hpp"

#include <iostream>
#include <mutex>
#include <string_view>

namespace crawler {
namespace {

std::mutex& log_mutex() {
    static std::mutex mutex;
    return mutex;
}

}  // namespace

bool is_directory_url(std::string_view url) {
    return !url.empty() && url.back() == '/';
}

bool should_follow(std::string_view href) {
    return href.find("/~vkepuska") != 0 && href.find('?') != 0;
}

std::filesystem::path local_path(std::string_view url) {
    const auto pos = url.find(kPathMarker);
    if (pos == std::string_view::npos) {
        return std::filesystem::path(".");
    }
    return std::filesystem::path(".") / std::string(url.substr(pos));
}

void log_line(std::string_view message) {
    std::lock_guard<std::mutex> lock(log_mutex());
    std::cout << message << '\n';
}

}  // namespace crawler
