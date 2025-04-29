#ifndef FMS_SOLVERS_ENVIRONMENTAL_SELECTION_OPERATOR_HPP
#define FMS_SOLVERS_ENVIRONMENTAL_SELECTION_OPERATOR_HPP

#include "partial_solution.hpp"

#include <vector>

namespace fms::solvers {

/**
 * @class EnvironmentalSelectionOperator
 * @brief Class for handling environmental selection operations.
 *
 * This class provides a method for reducing a set of partial solutions to a smaller set.
 * The number of solutions to keep is determined by the intermediate_solutions parameter.
 */
class EnvironmentalSelectionOperator {
    unsigned int intermediate_solutions; ///< The number of solutions to keep after reduction.

public:
    /**
     * @brief Constructor for the EnvironmentalSelectionOperator class.
     *
     * @param intermediate_solutions The number of solutions to keep after reduction.
     */
    explicit EnvironmentalSelectionOperator(unsigned int intermediate_solutions);

    /**
     * @brief Reduces a set of partial solutions to a smaller set.
     *
     * @param values The set of partial solutions.
     * @return The reduced set of partial solutions.
     */
    [[nodiscard]] std::vector<PartialSolution> reduce(std::vector<PartialSolution> values) const;
};

} // namespace fms::solvers

#endif // FMS_SOLVERS_ENVIRONMENTAL_SELECTION_OPERATOR_HPP