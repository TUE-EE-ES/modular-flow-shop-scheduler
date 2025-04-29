#ifndef FMS_SOLVERS_COCKTAIL_LINE_SOLVER_HPP
#define FMS_SOLVERS_COCKTAIL_LINE_SOLVER_HPP

#include "modular_args.hpp"
#include "production_line_solution.hpp"

#include "fms/problem/production_line.hpp"
#include "fms/solvers/distributed_scheduler_history.hpp"

#include <cstdint>
#include <nlohmann/json.hpp>
#include <vector>

namespace fms::solvers::CocktailLineSolver {
struct SingleIterationResult {
    ModulesSolutions modulesResults;
    bool converged;

    /// @brief Error message if the algorithm did not converge
    /// @details If the algorithm did not converge, this is not an empty string and
    /// contains the reason why. @p modulesResults is empty in this case.
    std::string error;
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
 * @brief Perform a single iteration (forward and backwards) of the cocktail algorithm.
 *
 * @param instance Instance to be solved
 * @param args Command line arguments
 * @param iterations Number of iterations performed so far
 * @param convergedLowerBound If true, the lower bounds have converged
 * @param argsMod Command line arguments for the modular algorithm
 * @param history History parameter to store the bounds and other information during the run.
 * @return SingleIterationResult Results of the single iteration
 */
SingleIterationResult singleIteration(problem::ProductionLine &instance,
                                      const cli::CLIArgs &args,
                                      std::uint64_t iterations,
                                      bool convergedLowerBound,
                                      const cli::ModularArgs &argsMod,
                                      DistributedSchedulerHistory &history);

} // namespace fms::solvers::CocktailLineSolver

#endif // FMS_SOLVERS_COCKTAIL_LINE_SOLVER_HPP
