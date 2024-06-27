#ifndef FORPFSSPSD_BOUNDS_HPP
#define FORPFSSPSD_BOUNDS_HPP

#include "boundary.hpp"
#include "indices.hpp"

#include <nlohmann/json.hpp>
#include <unordered_map>

namespace FORPFSSPSD {

using IntervalSpec = std::unordered_map<JobId, std::unordered_map<JobId, TimeInterval>>;

struct ModuleBounds {
    /// Intervals at the input boundary
    FORPFSSPSD::IntervalSpec in;

    /// Intervals at the output boundary
    FORPFSSPSD::IntervalSpec out;

    [[nodiscard]] bool operator==(const ModuleBounds &other) const {
        return in == other.in && out == other.out;
    }
};

using GlobalBounds = std::unordered_map<FORPFSSPSD::ModuleId, ModuleBounds>;

[[nodiscard]] nlohmann::json toJSON(const FORPFSSPSD::IntervalSpec &bounds);
[[nodiscard]] nlohmann::json toJSON(const FS::GlobalBounds &globalBounds);
[[nodiscard]] nlohmann::json toJSON(const std::vector<FS::GlobalBounds> &bounds);

FORPFSSPSD::IntervalSpec moduleBoundsFromJSON(const nlohmann::json &json);
FORPFSSPSD::GlobalBounds globalBoundsFromJSON(const nlohmann::json &json);
std::vector<FORPFSSPSD::GlobalBounds> allGlobalBoundsFromJSON(const nlohmann::json &json);
} // namespace FORPFSSPSD

#endif // FORPFSSPSD_BOUNDS_HPP
