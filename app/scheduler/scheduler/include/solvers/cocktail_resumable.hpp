#ifndef COCKTAIL_RESUMABLE_SOLVER_HPP
#define COCKTAIL_RESUMABLE_SOLVER_HPP

#include "FORPFSSPSD/production_line.hpp"
#include "modular_args.hpp"
#include "production_line_solution.hpp"
#include "solver_data.hpp"

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace algorithm::CocktailResumableSolver {

using SolversData = std::unordered_map<FORPFSSPSD::ModuleId, FMS::SolverDataPtr>;

struct SingleIterationResult {
    FMS::ModulesSolutions modulesResults;
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
std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
solve(FORPFSSPSD::ProductionLine &problemInstance, const commandLineArgs &args);

/**
 * @brief Perform a single iteration (forward and backwards) of the cocktail algorithm.
 *
 * @param instance Instance to be solved
 * @param args Command line arguments
 * @param iterations Number of iterations performed so far
 * @param convergedLowerBound If true, the lower bounds have converged
 * @param argsMod Command line arguments for the modular algorithm
 * @return SingleIterationResult Results of the single iteration
 */
SingleIterationResult singleIteration(FORPFSSPSD::ProductionLine &instance,
                                      const commandLineArgs &args,
                                      const ModularArgs &argsMod,
                                      std::uint64_t iterations,
                                      bool convergedLowerBound,
                                      SolversData &&solversData);

} // namespace algorithm::CocktailResumableSolver

#endif // COCKTAIL_RESUMABLE_SOLVER_HPP
