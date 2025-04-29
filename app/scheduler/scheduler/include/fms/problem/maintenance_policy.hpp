#ifndef PROBLEM_MAINTENANCE_POLICY_HPP
#define PROBLEM_MAINTENANCE_POLICY_HPP

#include "aliases.hpp"

#include "fms/delay.hpp"

#include <fmt/base.h>

namespace fms::problem {

/**
 * @class MaintenancePolicy
 * @brief Class representing a maintenance policy in the system.
 */
class MaintenancePolicy {
private:
    unsigned int numberOfTypes; ///< The number of types of maintenance
    delay minimumIdle;          ///< The minimum idle time

    std::map<MaintType, delay> maintDuration; ///< Map of maintenance types to their durations
    delay defaultMaintDuration;               ///< Default maintenance duration

    std::map<MaintType, std::tuple<delay, delay>>
            thresholds;     ///< Map of maintenance types to their thresholds
    delay defaultThreshold; ///< Default threshold

public:
    /// Default constructor
    MaintenancePolicy() = default;

    /**
     * @brief Constructor with parameters
     * @param numberOfTypes The number of types of maintenance
     * @param minimumIdle The minimum idle time
     * @param maintDuration Map of maintenance types to their durations
     * @param defaultMaintDuration Default maintenance duration
     * @param thresholds Map of maintenance types to their thresholds
     * @param defaultThreshold Default threshold
     */
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

    /// @return The number of types of maintenance
    [[nodiscard]] unsigned int getNumberOfTypes() const { return numberOfTypes; }

    /**
     * @brief Get the duration of a specific type of maintenance
     * @param id The id of the maintenance type
     * @return The duration of the maintenance type
     */
    [[nodiscard]] delay getMaintDuration(MaintType id) const;

    [[nodiscard]] inline delay getMaintDuration(const problem::Operation &op) const {
        return getMaintDuration(*op.maintId);
    }

    /// @return The minimum idle time
    [[nodiscard]] delay getMinimumIdle() const { return minimumIdle; }

    /**
     * @brief Get the thresholds of a specific type of maintenance
     * @param id The id of the maintenance type
     * @return The thresholds of the maintenance type
     */
    [[nodiscard]] std::tuple<delay, delay> getThresholds(MaintType id) const;
};

} // namespace fms::problem

/**
 * @brief Formatter for the MaintenancePolicy class
 */
template <> struct fmt::formatter<fms::problem::MaintenancePolicy> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const fms::problem::MaintenancePolicy &p,
                FormatContext &ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(),
                              "number of types : {}number of jobs : {}defaultMaintDuration : "
                              "{}defaultThreshold : {}\n",
                              p.numberOfTypes,
                              p.defaultMaintDuration,
                              p.defaultThreshold);
    }
};

#endif // PROBLEM_MAINTENANCE_POLICY_HPP
