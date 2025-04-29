#ifndef FMS_SOLVERS_SCHEDULING_OPTION_HPP
#define FMS_SOLVERS_SCHEDULING_OPTION_HPP

#include "fms/problem/operation.hpp"

#include <cstdint>

namespace fms::solvers {

/**
 * @struct option
 * @brief Struct for handling scheduling options.
 *
 * This struct serves as an option to add an operation somewhere in a schedule
 * or to remove an operation from somewhere in a schedule.
 */
struct SchedulingOption {

    /**
     * @brief Constructor for the option struct.
     *
     * @param prevO The previous operation in the sequence of operations.
     * @param curO The current operation being inserted in the sequence of operations.
     * @param nextO The next operation in the sequence of operations.
     * @param position The index of the insertion point of the current operation @p curO in the
     * sequence of operations.
     * @param is_maint A boolean indicating whether the current vertex is a maintenance vertex.
     */
    SchedulingOption(problem::Operation prevO,
                     problem::Operation curO,
                     problem::Operation nextO,
                     std::size_t position,
                     bool is_maint = false) :
        prevO(prevO), curO(curO), nextO(nextO), position(position), is_maint(is_maint) {}

    SchedulingOption(const SchedulingOption &other) = default;

    SchedulingOption(SchedulingOption &&other) = default;

    ~SchedulingOption() = default;

    SchedulingOption &operator=(const SchedulingOption &other) = default;

    SchedulingOption &operator=(SchedulingOption &&other) = default;

    ///< The ID.
    std::uint64_t id{};

    /// Weight of the current option. Used for ranking.
    long double weight{};

    /// The previous operation in the sequence of operations.
    problem::Operation prevO;

    /// The current operation being inserted in the sequence of operations.
    problem::Operation curO;

    /// The next operation in the sequence of operations.
    problem::Operation nextO;

    /// The insertion point of the current operation in the sequence of operations.
    std::size_t position;

    ///< A boolean indicating whether the current vertex is a maintenance vertex.
    bool is_maint;
};
} // namespace fms::solvers
#endif // FMS_SOLVERS_SCHEDULING_OPTION_HPP