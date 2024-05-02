#ifndef UTILS_HASH_HPP
#define UTILS_HASH_HPP

#include <functional>

namespace algorithm {
/**
 * @brief Combines two hash values. Obtained from: https://stackoverflow.com/a/2595226/4005637
 *
 * @tparam T
 */
template <class T> inline constexpr std::size_t hash_combine(std::size_t seed, const T &v) {
    const std::hash<T> hasher;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    return seed ^ hasher(v) + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
}

} // namespace algorithm

#endif // UTILS_HASH_HPP