#ifndef DELAYGRAPH_BUILDER_HPP
#define DELAYGRAPH_BUILDER_HPP

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "delayGraph/delayGraph.h"

namespace DelayGraph::Builder {

[[nodiscard]] delayGraph customOrder(const FORPFSSPSD::Instance &instance,
                                     const std::vector<FORPFSSPSD::JobId> &jobOrder);

[[nodiscard]] inline delayGraph FORPFSSPSD(const FORPFSSPSD::Instance &problemInstance) {
    return customOrder(problemInstance, problemInstance.getJobsOutput());
}

[[nodiscard]] delayGraph jobShop(const FORPFSSPSD::Instance &problemInstance);

/**
 * @brief Builds a constraint graph from a problem instance.
 * 
 * It selects the proper builder based on the problem type.
 * 
 * @param problemInstance Instance of the problem to build the delay graph from.
 * @return delayGraph Constraint-graph model representing the problem instance.
 */
[[nodiscard]] inline delayGraph build(const FORPFSSPSD::Instance &problemInstance) {
    switch (problemInstance.shopType()) {
    case ShopType::FIXEDORDERSHOP:
        return FORPFSSPSD(problemInstance);
    case ShopType::JOBSHOP:
        return jobShop(problemInstance);
    default:
        LOG_E("DelayGraph::Builder::build: Unknown problem type");
        throw std::runtime_error("Unknown problem type");
    }
}

} // namespace DelayGraph::Builder

#endif // DELAYGRAPH_BUILDER_HPP