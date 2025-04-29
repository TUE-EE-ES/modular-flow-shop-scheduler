#include "fms/pch/containers.hpp"
#include "fms/pch/utils.hpp"

#include "fms/problem/maintenance_policy.hpp"

#include "fms/problem/operation.hpp"

namespace fms::problem {

delay MaintenancePolicy::getMaintDuration(MaintType id) const {
    auto it = maintDuration.find(id);
    if (it == maintDuration.end()) {
        return defaultMaintDuration;
    }
    return it->second;
}

std::tuple<delay, delay> MaintenancePolicy::getThresholds(MaintType id) const {
    auto it = thresholds.find(id);
    if (it == thresholds.end()) {
        return std::make_tuple(defaultThreshold, defaultThreshold);
    }
    return it->second;
}
}; // namespace fms::problem
