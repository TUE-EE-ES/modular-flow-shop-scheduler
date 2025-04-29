#ifndef FMS_CG_BUILDER_HPP
#define FMS_CG_BUILDER_HPP

#include "constraint_graph.hpp"

#include "fms/problem/flow_shop.hpp"

namespace fms::cg::Builder {

/**
 * @brief Builds a delay graph with a custom job order
 * @param instance The problem instance
 * @param jobOrder The custom job order
 * @return The constructed delay graph
 */
[[nodiscard]] ConstraintGraph customOrder(const problem::Instance &instance,
                                          const std::vector<problem::JobId> &jobOrder);

/**
 * @brief Builds a delay graph for a FORPFSSPSD problem instance
 * @param problemInstance The problem instance
 * @return The constructed delay graph
 */
[[nodiscard]] inline ConstraintGraph FORPFSSPSD(const problem::Instance &problemInstance) {
    return customOrder(problemInstance, problemInstance.getJobsOutput());
}

/**
 * @brief Builds a delay graph for a job shop problem instance
 * @param problemInstance The problem instance
 * @return The constructed delay graph
 */
[[nodiscard]] ConstraintGraph jobShop(const problem::Instance &problemInstance);

/**
 * @brief Builds a constraint graph from a problem instance.
 * @details It selects the proper builder based on the problem type.
 * @param problemInstance Instance of the problem to build the delay graph from.
 * @return ConstraintGraph Constraint-graph model representing the problem instance.
 */
[[nodiscard]] inline ConstraintGraph build(const problem::Instance &problemInstance) {
    switch (problemInstance.shopType()) {
    case cli::ShopType::FIXEDORDERSHOP:
        return FORPFSSPSD(problemInstance);
    case cli::ShopType::JOBSHOP:
        return jobShop(problemInstance);
    default:
        LOG_E("cg::Builder::build: Unknown problem type");
        throw std::runtime_error("Unknown problem type");
    }
}

} // namespace fms::cg::Builder

#endif // FMS_CG_BUILDER_HPP