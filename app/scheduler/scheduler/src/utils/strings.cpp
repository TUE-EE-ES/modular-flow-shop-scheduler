#include "fms/pch/containers.hpp"

#include "fms/utils/strings.hpp"

namespace fms::utils::strings {

std::string toLower(const std::string_view input) {
    std::string s(input);
    trim(s);
    std::ranges::transform(s, s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

void lTrim(std::string &s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](int ch) { return std::isspace(ch) == 0; }));
}

void rTrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return std::isspace(ch) == 0; }).base(),
            s.end());
}

void trim(std::string &s) {
    lTrim(s);
    rTrim(s);
}

std::vector<std::string_view> split(std::string_view input, char delimiter) {
    auto r = std::ranges::views::split(input, delimiter)
             | std::ranges::views::transform([](auto &&range) {
                   return std::string_view(&*range.begin(), std::ranges::distance(range));
               })
             | std::ranges::views::filter([](const std::string_view &s) { return !s.empty(); });
    return {r.begin(), r.end()};
}

} // namespace fms::utils::strings
