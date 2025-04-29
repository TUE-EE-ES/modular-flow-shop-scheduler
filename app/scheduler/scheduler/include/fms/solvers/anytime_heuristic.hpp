#ifndef FMS_SOLVERS_ANYTIME_HEURISTIC_HPP
#define FMS_SOLVERS_ANYTIME_HEURISTIC_HPP

#include "scheduling_option.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/vertex.hpp"
#include "fms/delay.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"

#include <cstddef>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace fms::solvers::anytime {
/* Anytime counterpart of fowardheuristic's createOptions
Gets the next possible option for inserting a particular job based on a decided
way to search the option space
Currently just listed in order */
std::optional<std::pair<SchedulingOption, size_t>>
getNextOption(std::vector<SchedulingOption> options, size_t index);

/* Anytime counterpart of fowardheuristic's rankSolutions
 Gets the next possible option for inserting a particular job based on a decided
 way to search the option space
 Currently just listed in order */

std::tuple<std::tuple<delay, delay, delay, delay, std::uint32_t, std::uint32_t>,
           std::tuple<PartialSolution, delay, delay, std::uint32_t>>
rankSolution(std::tuple<PartialSolution, SchedulingOption> solution,
             cg::ConstraintGraph &dg,
             std::tuple<delay, delay, delay, delay, std::uint32_t, std::uint32_t> existingNorms,
             std::tuple<PartialSolution, delay, delay, std::uint32_t> existingRank,
             algorithms::paths::PathTimes &ASAPTimes,
             problem::MachineId reEntrantMachine,
             const cli::CLIArgs &args);

/* Algorithmic implementations */
PartialSolution scheduleOneOperation(cg::ConstraintGraph &dg,
                                     problem::Instance &,
                                     const PartialSolution &current_solution,
                                     const cg::Vertex &,
                                     const cli::CLIArgs &args);

PartialSolution getSolution(cg::ConstraintGraph &dg,
                            const problem::Instance &problem,
                            const cg::Vertex &eligibleOperation,
                            const PartialSolution &solution,
                            const cli::CLIArgs &args);

std::optional<std::pair<PartialSolution, SchedulingOption>>
evaluateOption(cg::ConstraintGraph &dg,
               const problem::Instance &problem,
               const cg::Vertex &eligibleOperation,
               const PartialSolution &solution,
               const SchedulingOption &o,
               const problem::Operation &lastPotentiallyFeasibleOption,
               std::tuple<delay, delay, delay, delay, uint32_t, uint32_t> &existingNorms,
               std::tuple<PartialSolution, delay, delay, uint32_t> &existingRank,
               const cli::CLIArgs &args);

cg::Edges getExitEdges();

PartialSolution solve(problem::Instance &problemInstance, const cli::CLIArgs &args);

} // namespace fms::solvers::anytime

#endif // FMS_SOLVERS_ANYTIME_HEURISTIC_HPP