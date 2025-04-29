#ifndef FMS_SOLVERS_BROADCAST_LINE_SOLVER_HPP
#define FMS_SOLVERS_BROADCAST_LINE_SOLVER_HPP

#include "production_line_solution.hpp"

#include "fms/problem/bounds.hpp"
#include "fms/problem/module.hpp"
#include "fms/problem/production_line.hpp"
#include "fms/solvers/algorithms_data.hpp"
#include "fms/solvers/distributed_scheduler_history.hpp"

#include <cstdint>
#include <nlohmann/json.hpp>
#include <vector>

namespace fms::solvers {

enum class BoundsSide { INPUT, OUTPUT, BOTH };

namespace BroadcastLineSolver {
class ErrorStrings {
public:
    static constexpr auto kNoConvergence = "no-convergence";
    static constexpr auto kLocalScheduler = "local-scheduler";
    static constexpr auto kTimeOut = "time-out";
};

/**
 * @brief Generate a schedule for the modular re-entrant flow-shop problem using the cocktail
 * constraint propagation algorithm.
 *
 * @param problemInstance Instance of the problem
 * @param args Command line arguments as parsed by commandLine::getArgs
 * @return std::tuple<std::vector<PartialSolution>, nlohmann::json> Vector of possible schedules
 * as well as additional information from the execution and solution
 */
std::tuple<std::vector<ProductionLineSolution>, nlohmann::json>
solve(problem::ProductionLine &problemInstance, const cli::CLIArgs &args);

/**
 * @brief Get the intervals between consecutive jobs at the input and output boundary of a
 * module for the given scheduling sequences. If multiple sequences are given the intervals are
 * extended to include all the options.
 *
 * @param problemInstance Module to be evaluated. The input boundary is the first operation and
 * the output boundary is the last operation processed in this module.
 * @param solutions Vector of feasible solutions to be evaluated.
 * @param upperBound If `true` the upper bound based on the scheduler is returned, otherwise
 * only the static upper bounds are returned.
 * @param intervalSide Side of the interval to be returned. If BOTH is given, both input and
 * output intervals are returned. Otherwise only the specified side is returned.
 * @return ModuleIntervals object containing the intervals at the input and output boundary.
 */
problem::ModuleBounds getBounds(const problem::Module &problemInstance,
                                const PartialSolution &solutions,
                                bool upperBound = false,
                                BoundsSide intervalSide = BoundsSide::BOTH);

/**
 * @brief Propagates the intervals between modules and checks if the problem changed.
 * @details The intervals are propagated from the input boundary of the module to the output
 * boundary of the following module and vice versa.
 * @param problemInstance Global problem instance.
 * @param translatedIntervals Translated intervals from the previous module.
 */
void propagateIntervals(problem::ProductionLine &problemInstance,
                        const problem::GlobalBounds &translatedIntervals);

std::tuple<problem::GlobalBounds, bool>
translateBounds(const problem::ProductionLine &problemInstance,
                const problem::GlobalBounds &intervals);

/**
 * @brief Merges feasible solutions from each of the modules into a feasible global solution.
 *
 * @param problemInstance Global problem instance.
 * @param solutions Vector of feasible solutions from each module.
 * @return fms::ProductionLineSolution Feasible solution to the global problem.
 */
ProductionLineSolution mergeSolutions(const problem::ProductionLine &problemInstance,
                                      ModulesSolutions &solutions);

/**
 * @brief Compare two sets of intervals and check if they are converged.
 * @details Because due dates between modules may not exist, this means that some translated
 * bounds may not exist, while the original bound does. This check compares the bounds and,
 * if one of them is not specified and the other is, returns true. This is different from what
 * std::optional does.
 *
 * @param sender Translated set of bounds from the sender module
 * @param receiver Set of bounds from the receiver module
 * @return true The bounds from both sides are "similar" and convergence has been achieved.
 * @return false The bounds from both sides are different and convergence has not been achieved.
 */
bool isConverged(const problem::IntervalSpec &sender, const problem::IntervalSpec &receiver);

nlohmann::json baseResultData(const DistributedSchedulerHistory &history,
                              const problem::ProductionLine &problem,
                              std::uint64_t iterations);
} // namespace BroadcastLineSolver
} // namespace fms::solvers

#endif // FMS_SOLVERS_BROADCAST_LINE_SOLVER_HPP
