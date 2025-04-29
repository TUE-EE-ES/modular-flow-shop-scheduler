#ifndef FMS_SOLVERS_FORWARD_HEURISTIC_HPP
#define FMS_SOLVERS_FORWARD_HEURISTIC_HPP

#include "partial_solution.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/edge.hpp"
#include "fms/delay.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/cli/command_line.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace fms::solvers::forward {

PartialSolution solve(problem::Instance &problemInstance, const cli::CLIArgs &args);
Sequence createInitialSequence(const problem::Instance &problemInstance,
                               problem::MachineId reEntrantMachine);

/**
 * @brief Find insertion point options for the given operation.
 * @details Given an operation @p eligibleOperation, this function finds all possible insertion
 * points to the current sequence.
 * @param dg Constraint graph associated to the problem. Used for the feasibility check.
 * @param problem Instance of the problem.
 * @param solution Current solution so far.
 * @param eligibleOperation Operation to insert.
 * @param reEntrantMachineId ID of the re-entrant machine that we are evaluating.
 * @return std::pair<problem::Operation, std::vector<SchedulingOption>>  The first element is the
 * operation of the last insertion point and the second, the list of possible insertion points
 * options.
 */
std::pair<problem::Operation, std::vector<SchedulingOption>>
createOptions(const problem::Instance &problem,
              const PartialSolution &solution,
              const cg::Vertex &eligibleOperation,
              problem::MachineId reEntrantMachineId);

std::vector<std::pair<PartialSolution, SchedulingOption>>
evaluateOptionFeasibility(cg::ConstraintGraph &dg,
                          const problem::Instance &problem,
                          const PartialSolution &solution,
                          const std::vector<SchedulingOption> &options,
                          const std::vector<delay> &ASAPTimes,
                          problem::MachineId reEntrantMachine);

std::optional<std::pair<PartialSolution, SchedulingOption>>
evaluateOptionFeasibility(cg::ConstraintGraph &dg,
                          const problem::Instance &problem,
                          const PartialSolution &solution,
                          const SchedulingOption &options,
                          const std::vector<delay> &ASAPTimes,
                          problem::MachineId reEntrantMachine);

/* Algorithmic implementations */

PartialSolution scheduleOneOperation(cg::ConstraintGraph &dg,
                                     problem::Instance &,
                                     const PartialSolution &current_solution,
                                     const problem::Operation &eligibleOperation,
                                     const cli::CLIArgs &args);

delay determineSmallestDeadline(const cg::Vertex &v);

/* add edges for the interleaving and validate whether the bound still hold; returns a negative
 * cycle if the resulting graph contains one (or a list of infeasible edges) */
algorithms::paths::LongestPathResult validateInterleaving(cg::ConstraintGraph &dg,
                                                          const problem::Instance &problem,
                                                          const cg::Edges &inputEdges,
                                                          std::vector<delay> &ASAPST,
                                                          const cg::VerticesCRef &sources,
                                                          const cg::VerticesCRef &window);

std::pair<delay, unsigned int> computeFutureAvgProductivy(cg::ConstraintGraph &dg,
                                                          const std::vector<delay> &ASAPST,
                                                          const PartialSolution &ps,
                                                          problem::MachineId reEntrantMachineId);

/**
 * @brief Ranks the given solutions and returns the index of the last one. Note that the
 *        solutions are not sorted.
 *
 * @param solutions Solutions to rank
 * @param ASAPTimes Start times of sequence before creating new options.
 * @param args Command line arguments. The user can select the exact weights of the ranking.
 * @param reEntrantMachine ID of the re-entrant machine whose operations are going to be ranked.
 * @return Index of the best solution and nullopt if no solution was found.
 */
std::optional<std::size_t> rankSolutions(
        std::vector<
                std::tuple<PartialSolution, SchedulingOption, std::shared_ptr<cg::ConstraintGraph>>>
                &solutions,
        algorithms::paths::PathTimes &ASAPTimes,
        problem::MachineId reEntrantMachine,
        const cli::CLIArgs &args);

/**
 * @brief Same as rankSolutions above bur ranks only based on ASAP requirements
 *
 */
std::optional<std::size_t> rankSolutionsASAP(
        std::vector<
                std::tuple<PartialSolution, SchedulingOption, std::shared_ptr<cg::ConstraintGraph>>>
                &solutions);

/**
 * @brief Find all feasible insertion points and their rank.
 *
 * @param dg Current delay graph
 * @param problem Problem instance
 * @param eligibleOperation Operation to insert
 * @param solution Current solution
 * @param args Command line arguments
 * @return std::tuple<std::vector<PartialSolution>, std::size_t> Vector of feasible solutions
 * and the index of the best one.
 */
std::tuple<std::vector<std::tuple<PartialSolution, std::shared_ptr<cg::ConstraintGraph>>>,
           std::optional<std::size_t>>
getFeasibleOptions(cg::ConstraintGraph &dg,
                   problem::Instance &problem,
                   const cg::Vertex &eligibleOperation,
                   const PartialSolution &solution,
                   const cli::CLIArgs &args);

std::uint32_t countOpsInBuffer(const PartialSolution &ps, problem::MachineId reEntrantMachineId);
} // namespace fms::solvers::forward

#endif // FMS_SOLVERS_FORWARD_HEURISTIC_HPP
