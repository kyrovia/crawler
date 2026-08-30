#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace crawler {

std::vector<std::string> extract_hrefs(std::string_view html);

}  // namespace crawler
