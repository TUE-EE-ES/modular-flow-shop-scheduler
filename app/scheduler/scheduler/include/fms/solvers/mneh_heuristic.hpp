#ifndef FMS_SOLVERS_MNEH_HEURISTIC_HPP
#define FMS_SOLVERS_MNEH_HEURISTIC_HPP

#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/edge.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/cli/command_line.hpp"

#include <tuple>

namespace fms::solvers {

class MNEH {
    static Sequence improveSequence(problem::Instance &problemInstance,
                                   problem::MachineId reEntrantMachine,
                                   const Sequence &seedSequence,
                                   cg::ConstraintGraph &dg,
                                   const cli::CLIArgs &args);

    static std::tuple<Sequence, PartialSolution>
    updateSequence(const problem::Instance &problemInstance,
                   problem::MachineId reEntrantMachine,
                   const Sequence &seedSequence,
                   cg::ConstraintGraph &dg,
                   const cli::CLIArgs &args);

    static bool validateSequence(const problem::Instance &problemInstance,
                                 const Sequence &sequence,
                                 problem::MachineId reEntrantMachine,
                                 const cg::ConstraintGraph &dg);

public:
    static PartialSolution solve(problem::Instance &problemInstance, const cli::CLIArgs &args);
};
} // namespace fms::solvers

#endif // FMS_SOLVERS_MNEH_HEURISTIC_HPP
