#ifndef UTILS_STRINGS_HPP
#define UTILS_STRINGS_HPP

#include <string>
#include <string_view>

namespace FMS {

/**
 * @brief Trims the string and then converts it to lower case.
 */
std::string toLower(std::string_view input);

void lTrim(std::string &s);

void rTrim(std::string &s);

void trim(std::string &s);

} // namespace FMS

#endif // UTILS_STRINGS_HPP
