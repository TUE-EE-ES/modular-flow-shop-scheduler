#ifndef MODULAR_SOLVER_HPP
#define MODULAR_SOLVER_HPP

#include "FORPFSSPSD/module.hpp"
#include "FORPFSSPSD/production_line.hpp"
#include "production_line_solution.hpp"


namespace algorithm {

enum class BoundsSide { INPUT, OUTPUT, BOTH };

struct ModuleBounds {
    /// Intervals at the input boundary
    FORPFSSPSD::IntervalSpec in;

    /// Intervals at the output boundary
    FORPFSSPSD::IntervalSpec out;

    [[nodiscard]] bool operator==(const ModuleBounds &other) const {
        return in == other.in && out == other.out;
    }
};

using GlobalIntervals = std::unordered_map<FORPFSSPSD::ModuleId, ModuleBounds>;

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
ModuleBounds getBounds(const FORPFSSPSD::Module &problemInstance,
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
                               const GlobalIntervals &translatedIntervals);

std::tuple<GlobalIntervals, bool>
translateBounds(const FORPFSSPSD::ProductionLine &problemInstance,
                const GlobalIntervals &intervals);

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
 * @param boundsLeft Set of bounds to check
 * @param boundsRight Set of bounds to check
 * @return true The bounds from both sides are "similar" and convergence has been achieved.
 * @return false The bounds from both sides are different and convergence has not been achieved.
 */
bool isConverged(const FORPFSSPSD::IntervalSpec &boundsLeft,
                 const FORPFSSPSD::IntervalSpec &boundsRight);

nlohmann::json boundsToJSON(const FORPFSSPSD::IntervalSpec &bounds);
nlohmann::json boundsToJSON(const GlobalIntervals &globalBounds);
nlohmann::json boundsToJSON(const std::vector<GlobalIntervals> &bounds);

FORPFSSPSD::IntervalSpec moduleBoundsFromJSON(const nlohmann::json &json);
GlobalIntervals globalBoundsFromJSON(const nlohmann::json &json);
std::vector<GlobalIntervals> allGlobalBoundsFromJSON(const nlohmann::json &json);

nlohmann::json baseResultData(const std::vector<FMS::ModulesSolutions> &solutions,
                              const std::vector<GlobalIntervals> &bounds,
                              const FORPFSSPSD::ProductionLine &problem,
                              std::uint64_t iterations);
} // namespace BroadcastLineSolver
} // namespace algorithm

#endif // MODULAR_SOLVER_HPP
