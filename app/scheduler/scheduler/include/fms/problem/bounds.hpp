#ifndef FORPFSSPSD_BOUNDS_HPP
#define FORPFSSPSD_BOUNDS_HPP

#include "boundary.hpp"
#include "indices.hpp"

#include <nlohmann/json.hpp>
#include <unordered_map>

namespace fms::problem {

using IntervalSpec = std::unordered_map<JobId, std::unordered_map<JobId, TimeInterval>>;

struct ModuleBounds {
    /// Intervals at the input boundary
    problem::IntervalSpec in;

    /// Intervals at the output boundary
    problem::IntervalSpec out;

    [[nodiscard]] bool operator==(const ModuleBounds &other) const {
        return in == other.in && out == other.out;
    }
};

using GlobalBounds = std::unordered_map<problem::ModuleId, ModuleBounds>;

[[nodiscard]] nlohmann::json toJSON(const problem::IntervalSpec &bounds);
[[nodiscard]] nlohmann::json toJSON(const problem::GlobalBounds &globalBounds);
[[nodiscard]] nlohmann::json toJSON(const std::vector<problem::GlobalBounds> &bounds);

problem::IntervalSpec moduleBoundsFromJSON(const nlohmann::json &json);
problem::GlobalBounds globalBoundsFromJSON(const nlohmann::json &json);
std::vector<problem::GlobalBounds> allGlobalBoundsFromJSON(const nlohmann::json &json);
} // namespace fms::problem

#endif // FORPFSSPSD_BOUNDS_HPP
