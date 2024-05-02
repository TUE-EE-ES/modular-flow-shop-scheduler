#include "utils/strings.hpp"

std::string FMS::toLower(const std::string_view input) {
    std::string s(input);
    trim(s);
    std::ranges::transform(s, s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

void FMS::lTrim(std::string &s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](int ch) { return std::isspace(ch) == 0; }));
}

void FMS::rTrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return std::isspace(ch) == 0; }).base(),
            s.end());
}

void FMS::trim(std::string &s) {
    lTrim(s);
    rTrim(s);
}
