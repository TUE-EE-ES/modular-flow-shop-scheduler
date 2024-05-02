#ifndef SOLVER_HPP
#define SOLVER_HPP

/** Basic definitions for all solvers
 */

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/production_line.hpp"
#include "partialsolution.h"
#include "production_line_solution.hpp"
#include "utils/commandLine.h"

namespace algorithm {

using Solutions = std::vector<PartialSolution>;
using ProductionLineSolutions = std::vector<FMS::ProductionLineSolution>;

using SolverOutput = std::tuple<Solutions, nlohmann::json>;
using ModularSolverOutput = std::tuple<ProductionLineSolutions, nlohmann::json>;

/// Definitions for the different types of solvers
namespace definitions {

/**
 * @brief Solve the passed problem instance and return the sequences of operations per machine.
 * @param problem Instance of a n-re-entrant flow-shop problem with setup times, due dates, and/or
 * maintenance times.
 * @param args Command line arguments.
 */
using BasicSolver = SolverOutput (&)(FORPFSSPSD::Instance &problem, const commandLineArgs &args);

/**
 * @brief Solve the passed problem instances and return the sequences of operations per machine.
 * @param problem Definition of a distributed scheduling problem.
 * @param args Command line arguments.
 */
using BasicModularSolver = ModularSolverOutput (&)(FORPFSSPSD::ProductionLine &problem,
                                                   const commandLineArgs &args);

} // namespace definitions
} // namespace algorithm

#endif // SOLVER_HPP