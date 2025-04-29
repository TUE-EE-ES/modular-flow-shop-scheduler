#ifndef FMS_SOLVERS_NO_FIXED_ORDER_SOLUTION_HPP
#define FMS_SOLVERS_NO_FIXED_ORDER_SOLUTION_HPP

#include "partial_solution.hpp"

#include "fms/cg/constraint_graph.hpp"
#include "fms/problem/indices.hpp"

#include <utility>
#include <vector>

namespace fms::solvers {

/**
 * @class NoFixedOrderSolution
 * @brief Class for handling solutions with no fixed order.
 *
 * This class provides methods for getting and setting the delay graph of a solution,
 * and for constructing a solution with a given job order and partial solution.
 */
class NoFixedOrderSolution {
public:
    PartialSolution solution;
    std::vector<problem::JobId> jobOrder;

    /**
     * @brief Default constructor.
     */
    NoFixedOrderSolution() : solution({}, {}) {}

    /**
     * @brief Parameterised constructor.
     *
     * @param jobOrder The job order.
     * @param args The arguments for constructing the partial solution.
     */
    template <typename... Args>
    explicit NoFixedOrderSolution(std::vector<problem::JobId> jobOrder, Args &&...args) :
        solution(std::forward<Args>(args)...), jobOrder(std::move(jobOrder)) {}

    /**
     * @brief Parameterised constructor.
     *
     * @param jobOrder The job order.
     * @param solution The partial solution.
     */
    NoFixedOrderSolution(std::vector<problem::JobId> jobOrder, PartialSolution solution) :
        solution(std::move(solution)), jobOrder(std::move(jobOrder)) {}

    /**
     * @brief Gets the delay graph.
     *
     * @return The delay graph.
     */
    const cg::ConstraintGraph &getDelayGraph() const { return m_dg; }

    /**
     * @brief Updates the delay graph.
     *
     * @param newGraph The new delay graph.
     */
    void updateDelayGraph(const cg::ConstraintGraph &newGraph) { m_dg = newGraph; }

private:
    cg::ConstraintGraph m_dg; ///< The delay graph.
};

} // namespace fms::solvers

#endif // FMS_SOLVERS_NO_FIXED_ORDER_SOLUTION_HPP