#include "crawler/html.hpp"

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace crawler {
namespace {

constexpr bool ieq(char left, char right) {
    return std::tolower(static_cast<unsigned char>(left)) ==
           std::tolower(static_cast<unsigned char>(right));
}

std::size_t find_ci(std::string_view hay, std::string_view needle, std::size_t from) {
    if (needle.empty() || from > hay.size()) {
        return std::string_view::npos;
    }
    const std::size_t last = hay.size() - needle.size();
    for (std::size_t i = from; i <= last; ++i) {
        bool match = true;
        for (std::size_t j = 0; j < needle.size(); ++j) {
            if (!ieq(hay[i + j], needle[j])) {
                match = false;
                break;
            }
        }
        if (match) {
            return i;
        }
    }
    return std::string_view::npos;
}

std::string_view skip_ws(std::string_view text, std::size_t& pos) {
    while (pos < text.size() &&
           std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
        ++pos;
    }
    return text;
}

}  // namespace

std::vector<std::string> extract_hrefs(std::string_view html) {
    std::vector<std::string> hrefs;
    std::size_t pos = 0;
    while ((pos = find_ci(html, "href", pos)) != std::string_view::npos) {
        pos += 4;
        skip_ws(html, pos);
        if (pos >= html.size() || html[pos] != '=') {
            continue;
        }
        ++pos;
        skip_ws(html, pos);
        if (pos >= html.size()) {
            break;
        }

        char quote = '\0';
        if (html[pos] == '"' || html[pos] == '\'') {
            quote = html[pos];
            ++pos;
        }

        const std::size_t start = pos;
        if (quote != '\0') {
            while (pos < html.size() && html[pos] != quote) {
                ++pos;
            }
        } else {
            while (pos < html.size() &&
                   std::isspace(static_cast<unsigned char>(html[pos])) == 0 &&
                   html[pos] != '>') {
                ++pos;
            }
        }
        hrefs.emplace_back(html.substr(start, pos - start));
        if (quote != '\0' && pos < html.size()) {
            ++pos;
        }
    }
    return hrefs;
}

}  // namespace crawler
