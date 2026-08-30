#include "crawler/http.hpp"

#include <curl/curl.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace crawler {
namespace {

size_t write_string(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

size_t write_file(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::ofstream*>(userdata);
    out->write(ptr, static_cast<std::streamsize>(size * nmemb));
    return out->good() ? size * nmemb : 0;
}

class EasyHandle {
public:
    EasyHandle() : handle_(curl_easy_init()) {
        if (handle_ == nullptr) {
            throw std::runtime_error("curl_easy_init failed");
        }
    }

    ~EasyHandle() {
        curl_easy_cleanup(handle_);
    }

    EasyHandle(const EasyHandle&) = delete;
    EasyHandle& operator=(const EasyHandle&) = delete;

    CURL* get() const { return handle_; }

    void set_common(std::string_view url) {
        url_ = std::string(url);
        const CURLcode url_ok = curl_easy_setopt(handle_, CURLOPT_URL, url_.c_str());
        if (url_ok != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(url_ok));
        }
        curl_easy_setopt(handle_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(handle_, CURLOPT_USERAGENT, "crawler/1.0");
        curl_easy_setopt(handle_, CURLOPT_TIMEOUT, 60L);
    }

    void perform(std::string_view url) {
        const CURLcode code = curl_easy_perform(handle_);
        if (code != CURLE_OK) {
            throw std::runtime_error(std::string("GET ") + std::string(url) + ": " +
                                     curl_easy_strerror(code));
        }
        long status = 0;
        curl_easy_getinfo(handle_, CURLINFO_RESPONSE_CODE, &status);
        if (status >= 400) {
            throw std::runtime_error(std::string("GET ") + std::string(url) +
                                     " HTTP " + std::to_string(status));
        }
    }

private:
    CURL* handle_;
    std::string url_;
};

}  // namespace

CurlGlobal::CurlGlobal() {
    const CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (code != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(code));
    }
}

CurlGlobal::~CurlGlobal() {
    curl_global_cleanup();
}

std::string http_get(std::string_view url) {
    EasyHandle easy;
    std::string body;
    easy.set_common(url);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, write_string);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEDATA, &body);
    easy.perform(url);
    return body;
}

void http_download(std::string_view url, const std::filesystem::path& dest) {
    if (dest.has_parent_path()) {
        std::filesystem::create_directories(dest.parent_path());
    }

    std::ofstream out(dest, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot write " + dest.string());
    }

    EasyHandle easy;
    easy.set_common(url);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, write_file);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEDATA, &out);
    easy.perform(url);
}

}  // namespace crawler
