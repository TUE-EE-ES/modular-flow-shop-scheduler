#ifndef FMS_SOLVERS_PARETO_HEURISTIC_HPP
#define FMS_SOLVERS_PARETO_HEURISTIC_HPP

#include "partial_solution.hpp"
#include "scheduling_option.hpp"

#include "fms/cg/constraint_graph.hpp"
#include "fms/problem/flow_shop.hpp"

#include <utility>
#include <vector>

namespace fms::solvers {

class ParetoHeuristic {

public:
    static std::vector<PartialSolution> solve(problem::Instance &problemInstance,
                                              const cli::CLIArgs &args);

    /* Algorithmic implementations */
    static std::vector<PartialSolution>
    scheduleOneOperation(cg::ConstraintGraph &dg,
                         const problem::Instance &problem,
                         const std::vector<PartialSolution> &currentSolutions,
                         const cg::Vertex &eligibleOperation,
                         unsigned int maximumPartialSolutions);

    static std::pair<delay, unsigned int> countOperations(cg::ConstraintGraph &dg,
                                                          const PartialSolution &ps);

    static std::pair<delay, unsigned int>
    countOperations(cg::ConstraintGraph &dg, const PartialSolution &ps, const SchedulingOption &c);

    static delay determine_smallest_deadline(const cg::Vertex &v);
};
} // namespace fms::solvers
#endif // FMS_SOLVERS_PARETO_HEURISTIC_HPP
