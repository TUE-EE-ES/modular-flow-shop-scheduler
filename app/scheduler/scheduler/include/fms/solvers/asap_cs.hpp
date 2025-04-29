#ifndef FMS_SOLVERS_ASAPCS_HEURISTIC_HPP
#define FMS_SOLVERS_ASAPCS_HEURISTIC_HPP

#include "fms/cg/constraint_graph.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/cli/command_line.hpp"

#include <optional>
#include <tuple>

namespace fms::solvers {

class ASAPCS {
public:
    static PartialSolution solve(problem::Instance &problemInstance, const cli::CLIArgs &args);

private:
    static std::tuple<PartialSolution, std::size_t>
    scheduleOneOperation(cg::ConstraintGraph &dg,
                         problem::Instance &problem,
                         problem::MachineId reEntrantMachine,
                         const PartialSolution &currentSolution,
                         const problem::Operation &eligibleOperation,
                         std::optional<std::size_t> lastInsertionPoint);
};
} // namespace fms::solvers

#endif // FMS_SOLVERS_ASAPCS_HEURISTIC_HPP
