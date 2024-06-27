#ifndef SOLVER_HPP
#define SOLVER_HPP

/** Basic definitions for all solvers
 */

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/problem_update.hpp"
#include "FORPFSSPSD/production_line.hpp"
#include "partialsolution.h"
#include "production_line_solution.hpp"
#include "solvers/solver_data.hpp"
#include "utils/commandLine.h"

#include <nlohmann/json.hpp>
#include <tuple>
#include <vector>

namespace algorithm {

using Solutions = std::vector<PartialSolution>;
using ProductionLineSolutions = std::vector<FMS::ProductionLineSolution>;

using SolverOutput = std::tuple<Solutions, nlohmann::json>;
using ModularSolverOutput = std::tuple<ProductionLineSolutions, nlohmann::json>;
using ResumableSolverOutput = std::tuple<Solutions, nlohmann::json, FMS::SolverDataPtr>;

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

/**
 * @brief Solve the passed problem instance and return the sequences of operations per machine.
 * @param problem Instance of a n-re-entrant flow-shop problem with setup times, due dates, and/or
 * maintenance times.
 * @param update Problem update to be applied to the problem instance adding and/or removing
 * constraints.
 * @param args Command line arguments.
 * @param solverData Scheduler-dependent data that can be used to resume the solver.
 */
using ResumableSolver = ResumableSolverOutput (&)(FORPFSSPSD::Instance &problem,
                                                  const FORPFSSPSD::ProblemUpdate update,
                                                  const commandLineArgs &args,
                                                  FMS::SolverDataPtr data);

} // namespace definitions
} // namespace algorithm

#endif // SOLVER_HPP