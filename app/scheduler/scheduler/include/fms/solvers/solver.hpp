#ifndef SOLVER_HPP
#define SOLVER_HPP

/** Basic definitions for all solvers
 */

#include "partial_solution.hpp"
#include "production_line_solution.hpp"
#include "solver_data.hpp"

#include "fms/problem/flow_shop.hpp"
#include "fms/problem/problem_update.hpp"
#include "fms/problem/production_line.hpp"
#include "fms/cli/command_line.hpp"

#include <nlohmann/json.hpp>
#include <tuple>
#include <vector>

namespace fms::solvers {

using Solutions = std::vector<PartialSolution>;
using ProductionLineSolutions = std::vector<ProductionLineSolution>;

using SolverOutput = std::tuple<Solutions, nlohmann::json>;
using ModularSolverOutput = std::tuple<ProductionLineSolutions, nlohmann::json>;
using ResumableSolverOutput = std::tuple<Solutions, nlohmann::json, SolverDataPtr>;

/// Definitions for the different types of solvers
namespace definitions {

/**
 * @brief Solve the passed problem instance and return the sequences of operations per machine.
 * @param problem Instance of a n-re-entrant flow-shop problem with setup times, due dates, and/or
 * maintenance times.
 * @param args Command line arguments.
 */
using BasicSolver = SolverOutput (&)(problem::Instance &problem, const cli::CLIArgs &args);

/**
 * @brief Solve the passed problem instances and return the sequences of operations per machine.
 * @param problem Definition of a distributed scheduling problem.
 * @param args Command line arguments.
 */
using BasicModularSolver = ModularSolverOutput (&)(problem::ProductionLine &problem,
                                                   const cli::CLIArgs &args);

/**
 * @brief Solve the passed problem instance and return the sequences of operations per machine.
 * @param problem Instance of a n-re-entrant flow-shop problem with setup times, due dates, and/or
 * maintenance times.
 * @param update Problem update to be applied to the problem instance adding and/or removing
 * constraints.
 * @param args Command line arguments.
 * @param solverData Scheduler-dependent data that can be used to resume the solver.
 */
using ResumableSolver = ResumableSolverOutput (&)(problem::Instance &problem,
                                                  const problem::ProblemUpdate update,
                                                  const cli::CLIArgs &args,
                                                  SolverDataPtr data);

} // namespace definitions
} // namespace fms::solvers

#endif // SOLVER_HPP