#ifndef FMS_UTILS_CONTAINERS_HPP
#define FMS_UTILS_CONTAINERS_HPP

#include "default_map.hpp" // IWYU pragma: export

namespace fms::utils::containers {
template <typename TKey, typename TValue> using Map = ankerl::unordered_dense::map<TKey, TValue>;
} // namespace fms::utils::containers

#endif // FMS_UTILS_CONTAINERS_HPP