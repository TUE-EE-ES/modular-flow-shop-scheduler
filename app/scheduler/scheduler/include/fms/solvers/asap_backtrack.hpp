#ifndef FMS_SOLVERS_ASAP_BACKTRACK_HEURISTIC_HPP
#define FMS_SOLVERS_ASAP_BACKTRACK_HEURISTIC_HPP

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cli/command_line.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"

#include <optional>
#include <tuple>

namespace fms::solvers {

class AsapBacktrack {
public:
    static PartialSolution solve(problem::Instance &problemInstance, const cli::CLIArgs &args);

private:
    static std::optional<std::size_t>
    scheduleOneOperation(cg::ConstraintGraph &dg,
                         problem::Instance &problem,
                         const problem::Operation &eligibleOperation,
                         Sequence &currentSequence,
                         std::size_t lastInsertionPoint,
                         algorithms::paths::PathTimes &ASAPST);
};
} // namespace fms::solvers

#endif // FMS_SOLVERS_ASAP_BACKTRACK_HEURISTIC_HPP
