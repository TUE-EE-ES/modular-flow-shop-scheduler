#ifndef MAINTENANCE_POLICY_HPP
#define MAINTENANCE_POLICY_HPP

#include "FORPFSSPSD/aliases.hpp"
#include "delay.h"

#include "pch/containers.hpp"

#include <fmt/core.h>

namespace FORPFSSPSD {
class MaintenancePolicy {
private:
    unsigned int numberOfTypes;
    delay minimumIdle;

    std::map<MaintType, delay> maintDuration;
    delay defaultMaintDuration;

    std::map<MaintType, std::tuple<delay, delay>> thresholds;
    delay defaultThreshold;

public:
    MaintenancePolicy() = default;
    MaintenancePolicy(const unsigned int numberOfTypes,
                      delay minimumIdle,
                      std::map<MaintType, delay> maintDuration,
                      delay defaultMaintDuration,
                      std::map<MaintType, std::tuple<delay, delay>> thresholds,
                      delay defaultThreshold) :
        numberOfTypes(numberOfTypes),
        minimumIdle(minimumIdle),
        maintDuration(std::move(maintDuration)),
        defaultMaintDuration(defaultMaintDuration),
        thresholds(std::move(thresholds)),
        defaultThreshold(defaultThreshold) {}

    friend class fmt::formatter<MaintenancePolicy>;

    [[nodiscard]] unsigned int getNumberOfTypes() const { return numberOfTypes; }

    [[nodiscard]] delay getMaintDuration(MaintType id) const {
        auto it = maintDuration.find(id);
        if (it == maintDuration.end()) {
            return defaultMaintDuration;
        }
        return it->second;
    }

    [[nodiscard]] delay getMinimumIdle() const { return minimumIdle; }

    [[nodiscard]] std::tuple<delay, delay> getThresholds(MaintType id) const {
        auto it = thresholds.find(id);
        if (it == thresholds.end()) {
            return std::make_tuple(defaultThreshold, defaultThreshold);
        }
        return it->second;
    }
};
} // namespace FORPFSSPSD

template <> struct fmt::formatter<FORPFSSPSD::MaintenancePolicy> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const FORPFSSPSD::MaintenancePolicy &p, FormatContext &ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(),
                              "number of types : {}number of jobs : {}defaultMaintDuration : "
                              "{}defaultThreshold : {}\n",
                              p.numberOfTypes,
                              p.defaultMaintDuration,
                              p.defaultThreshold);
    }
};

#endif // MAINTENANCE_POLICY_HPP