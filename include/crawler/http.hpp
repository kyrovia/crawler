#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace crawler {

class CurlGlobal {
public:
    CurlGlobal();
    ~CurlGlobal();

    CurlGlobal(const CurlGlobal&) = delete;
    CurlGlobal& operator=(const CurlGlobal&) = delete;
};

std::string http_get(std::string_view url);
void http_download(std::string_view url, const std::filesystem::path& dest);

}  // namespace crawler
