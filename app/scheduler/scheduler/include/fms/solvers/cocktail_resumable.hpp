#ifndef COCKTAIL_RESUMABLE_SOLVER_HPP
#define COCKTAIL_RESUMABLE_SOLVER_HPP

#include "modular_args.hpp"
#include "production_line_solution.hpp"
#include "solver_data.hpp"

#include "fms/problem/production_line.hpp"

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace fms::solvers::cocktail_resumable {

using SolversData = std::unordered_map<problem::ModuleId, SolverDataPtr>;

struct SingleIterationResult {
    ModulesSolutions modulesResults;
    bool converged;

    /// @brief Error message if the algorithm did not converge
    /// @details If the algorithm did not converge, this is not an empty string and
    /// contains the reason why. @p modulesResults is empty in this case.
    std::string error;

    /// @brief Data from the solvers
    SolversData solversData;
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
 * @param solversData Solver-specific data from the resumable solvers to be used in the next
 * iteration. The solvers return this data after each iteration.
 * @return SingleIterationResult Results of the single iteration
 */
SingleIterationResult singleIteration(problem::ProductionLine &instance,
                                      const cli::CLIArgs &args,
                                      const cli::ModularArgs &argsMod,
                                      std::uint64_t iterations,
                                      bool convergedLowerBound,
                                      SolversData &&solversData);

} // namespace fms::solvers::cocktail_resumable

#endif // COCKTAIL_RESUMABLE_SOLVER_HPP
