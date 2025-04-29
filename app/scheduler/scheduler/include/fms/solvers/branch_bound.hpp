#ifndef FMS_SOLVERS_BRANCH_BOUND_HPP
#define FMS_SOLVERS_BRANCH_BOUND_HPP

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/solvers/solver.hpp"

#include <utility>
#include <vector>

namespace fms::solvers::branch_bound {

class BranchBoundNode {
public:
    BranchBoundNode(const problem::Instance &problem,
                    cg::ConstraintGraph &dg,
                    const PartialSolution &solution,
                    delay trivialLowerBound);

    [[nodiscard]] const PartialSolution &getSolution() const { return solution; }

    [[nodiscard]] delay getLowerbound() const { return lowerbound; }

    [[nodiscard]] delay getMakespan() const { return makespan; }

    [[nodiscard]] problem::Operation getLastInsertedOperation() const {
        return lastInsertedOperation;
    }

    algorithms::paths::PathTimes getASAPST(const problem::Instance &problem,
                                           cg::ConstraintGraph &dg) const;

private:
    PartialSolution solution;
    delay lowerbound;
    delay makespan;
    problem::Operation lastInsertedOperation;
};

bool operator>(const BranchBoundNode &lhs, const BranchBoundNode &rhs);

PartialSolution solve(problem::Instance &problemInstance, const cli::CLIArgs &args);
cg::Edges create_initial_sequence(const problem::Instance &problemInstance,
                                  unsigned int reentrant_machine);

delay createTrivialCompletionLowerBound(const problem::Instance &problem);
Solutions
ranked(cg::ConstraintGraph &dg,
       const problem::Instance &problem,
       const std::vector<std::pair<PartialSolution, SchedulingOption>> &generationOfSolutions,
       const std::vector<delay> &ASAPTimes);

Solutions scheduleOneOperation(cg::ConstraintGraph &dg,
                               const problem::Instance &,
                               const PartialSolution &current_solution,
                               const cg::Vertex &);

BranchBoundNode createStupidSchedule(const problem::Instance &problemInstance,
                                     problem::MachineId reentrant_machine,
                                     delay trivialLowerBound);
} // namespace fms::solvers::branch_bound

#endif // FMS_SOLVERS_BRANCH_BOUND_HPP
