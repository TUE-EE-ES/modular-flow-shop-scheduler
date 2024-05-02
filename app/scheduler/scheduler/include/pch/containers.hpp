#ifndef PCH_CONTAINERS_HPP
#define PCH_CONTAINERS_HPP

#include <list>             // IWYU pragma: export
#include <map>              // IWYU pragma: export
#include <optional>         // IWYU pragma: export
#include <queue>            // IWYU pragma: export
#include <set>              // IWYU pragma: export
#include <string>           // IWYU pragma: export
#include <tuple>            // IWYU pragma: export
#include <unordered_map>    // IWYU pragma: export
#include <unordered_set>    // IWYU pragma: export
#include <variant>          // IWYU pragma: export
#include <vector>           // IWYU pragma: export

#include <ankerl/unordered_dense.h> // IWYU pragma: export
#include <nlohmann/json.hpp> // IWYU pragma: export

namespace FMS {
template <typename TKey, typename TValue> using Map = ankerl::unordered_dense::map<TKey, TValue>;
} // namespace FMS

#endif // PCH_CONTAINERS_HPP