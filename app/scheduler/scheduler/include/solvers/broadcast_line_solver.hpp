#ifndef MODULAR_SOLVER_HPP
#define MODULAR_SOLVER_HPP

#include "FORPFSSPSD/bounds.hpp"
#include "FORPFSSPSD/module.hpp"
#include "FORPFSSPSD/production_line.hpp"
#include "production_line_solution.hpp"
#include "utils/distributed_scheduler_history.hpp"

#include <cstdint>
#include <nlohmann/json.hpp>
#include <vector>

namespace algorithm {

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
std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
solve(FORPFSSPSD::ProductionLine &problemInstance, const commandLineArgs &args);

/**
 * @brief Get the intervals between consecutive jobs at the input and output boundary of a
 * module for the given scheduling sequences. If multiple sequences are given the intervals are
 * extended to include all the options.
 *
 * @param problemInstance Module to be evaluated. The input boundary is the first operation and
 * the output boundary is the last operation processed in this module.
 * @param solutions Vector of feasible solutions to be evaluated.
 * @param boundType If BOTH is given the interval contains both bounds if exists, otherwise
 * the value is std::nullopt on the side not included.
 * @param upperBound If `true` the upper bound based on the scheduler is returned, otherwise
 * only the static upper bounds are returned.
 * @param intervalSide Side of the interval to be returned. If BOTH is given both input and
 * output intervals are returned. Otherwise only the specified side is returned.
 * @return ModuleIntervals object containing the intervals at the input and output boundary.
 */
FORPFSSPSD::ModuleBounds getBounds(const FORPFSSPSD::Module &problemInstance,
                                   const PartialSolution &solutions,
                                   bool upperBound = false,
                                   BoundsSide intervalSide = BoundsSide::BOTH);

/**
 * @brief Propagates the intervals between modules and checks if the problem changed.
 * @details The intervals are propagated from the input boundary of the module to the output
 * boundary of the following module and vice versa.
 * @param problemInstance
 * @param intervals
 */
void propagateIntervals(FORPFSSPSD::ProductionLine &problemInstance,
                        const FORPFSSPSD::GlobalBounds &translatedIntervals);

std::tuple<FORPFSSPSD::GlobalBounds, bool>
translateBounds(const FORPFSSPSD::ProductionLine &problemInstance,
                const FORPFSSPSD::GlobalBounds &intervals);

/**
 * @brief Merges feasible solutions from each of the modules into a feasible global solution.
 *
 * @param problemInstance Global problem instance.
 * @param solutions Vector of feasible solutions from each module.
 * @return FMS::ProductionLineSolution Feasible solution to the global problem.
 */
FMS::ProductionLineSolution mergeSolutions(const FORPFSSPSD::ProductionLine &problemInstance,
                                           FMS::ModulesSolutions &solutions);

/**
 * @brief Compare two sets of intervals and check if they are converged.
 * @details Because due dates between modules may not exist, this means that some translated
 * bounds may not exist, while the original bound does. This check compares the bounds and,
 * if one of them is not specified and the other is, returns true. This is different from what
 * std::optional does.
 *
 * @param boundsLeft Translated set of bounds from the sender module
 * @param boundsRight Set of bounds from the receiver module
 * @return true The bounds from both sides are "similar" and convergence has been achieved.
 * @return false The bounds from both sides are different and convergence has not been achieved.
 */
bool isConverged(const FORPFSSPSD::IntervalSpec &sender,
                 const FORPFSSPSD::IntervalSpec &receiver);

nlohmann::json baseResultData(const FMS::DistributedSchedulerHistory &history,
                              const FORPFSSPSD::ProductionLine &problem,
                              std::uint64_t iterations);
} // namespace BroadcastLineSolver
} // namespace algorithm

#endif // MODULAR_SOLVER_HPP
