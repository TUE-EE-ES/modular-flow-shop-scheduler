#ifndef FMS_SOLVERS_GEOMETRIC_SELECTION_OPERATOR_HPP
#define FMS_SOLVERS_GEOMETRIC_SELECTION_OPERATOR_HPP

#include "partial_solution.hpp"

namespace fms::solvers {

/**
 * @class GeometricSelectionOperator
 * @brief Class for handling geometric selection operations.
 *
 * This class provides methods for reducing a set of partial solutions to a smaller set
 * based on geometric selection. The number of solutions to keep is determined by the
 * intermediate_solutions parameter.
 */
class GeometricSelectionOperator {
    const unsigned int intermediate_solutions; ///< The number of solutions to keep after reduction.

private:
    /**
     * @brief Flattens the given partial solution.
     *
     * @param t The partial solution to flatten.
     * @return The flattened value as a double.
     */
    static inline double flatten(const PartialSolution &t) {
        return (double)t.getAverageProductivity() / 1e6 * (double)t.getMakespanLastScheduledJob()
               / 1e6;
    }

    /**
     * @brief Calculates the value angle of the given partial solution.
     *
     * @param t The partial solution to calculate the value angle for.
     * @return The value angle as a double.
     */
    static double valueAngle(const PartialSolution &t) { return atan(flatten(t)); }

    /**
     * @brief Compares two partial solutions.
     *
     * @param t1 The first partial solution.
     * @param t2 The second partial solution.
     * @return True if t1 is less than t2, false otherwise.
     */
    static inline bool compareEntries(const PartialSolution &t1, const PartialSolution &t2);

public:
    /**
     * @brief Constructor for the GeometricSelectionOperator class.
     *
     * @param intermediate_solutions The number of solutions to keep after reduction.
     */
    explicit GeometricSelectionOperator(const unsigned int intermediate_solutions);

    /**
     * @brief Reduces a set of partial solutions to a smaller set.
     *
     * @param values The set of partial solutions.
     * @return The reduced set of partial solutions.
     */
    std::vector<PartialSolution> reduce(std::vector<PartialSolution> values) const;
};

} // namespace fms::solvers

#endif // FMS_SOLVERS_GEOMETRIC_SELECTION_OPERATOR_HPP