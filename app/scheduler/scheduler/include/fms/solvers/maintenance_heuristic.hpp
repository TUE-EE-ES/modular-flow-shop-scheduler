#ifndef FMS_SOLVERS_MAINTENANCE_HEURISTIC_HPP
#define FMS_SOLVERS_MAINTENANCE_HEURISTIC_HPP

#include "fms/algorithms/longest_path.hpp"
#include "fms/problem/flow_shop.hpp"

/**
 * @brief Class for handling maintenance heuristics.
 *
 * This class provides methods for triggering maintenance, evaluating schedules, inserting
 * maintenance, fetching idle time, and checking intervals. It also provides a method for
 * recomputing schedules.
 */
namespace fms::solvers::maintenance {
/**
 * @brief Triggers maintenance on a machine.
 *
 * @param dg The delay graph.
 * @param problemInstance The problem instance.
 * @param machine The machine ID.
 * @param solution The partial solution.
 * @param args The command line arguments.
 * @return A tuple containing the new partial solution and the updated delay graph.
 */
std::tuple<PartialSolution, cg::ConstraintGraph>
triggerMaintenance(cg::ConstraintGraph dg,
                   problem::Instance &problemInstance,
                   problem::MachineId machine,
                   const PartialSolution &solution,
                   const cli::CLIArgs &args);

/**
 * @brief Triggers maintenance on an eligible option.
 *
 * @param dg The delay graph.
 * @param problemInstance The problem instance.
 * @param solution The partial solution.
 * @param eligibleOption The eligible option.
 * @param args The command line arguments.
 * @return A tuple containing the new partial solution and the updated delay graph.
 */
std::tuple<PartialSolution, cg::ConstraintGraph>
triggerMaintenance(cg::ConstraintGraph dg,
                   problem::Instance &problemInstance,
                   const PartialSolution &solution,
                   const SchedulingOption &eligibleOption,
                   const cli::CLIArgs &args);

/**
 * @brief Triggers maintenance between two operations.
 *
 * @param dg The delay graph.
 * @param problemInstance The problem instance.
 * @param solution The partial solution.
 * @param eligibleOperation The eligible operation.
 * @param nextOperation The next operation.
 * @param args The command line arguments.
 * @return A tuple containing the new partial solution and the updated delay graph.
 */
std::tuple<PartialSolution, cg::ConstraintGraph>
triggerMaintenance(cg::ConstraintGraph dg,
                   problem::Instance &problemInstance,
                   const PartialSolution &solution,
                   problem::Operation eligibleOperation,
                   problem::Operation nextOperation,
                   const cli::CLIArgs &args);

/**
 * @brief Evaluates a schedule.
 *
 * @param problemInstance The problem instance.
 * @param dg The delay graph.
 * @param schedule The schedule.
 * @param eligibleOperation The eligible operation.
 * @param nextOperation The next operation.
 * @param args The command line arguments.
 * @return A tuple containing the new partial solution and the updated delay graph.
 */
std::tuple<PartialSolution, cg::ConstraintGraph>
evaluateSchedule(problem::Instance &problemInstance,
                 cg::ConstraintGraph &dg,
                 const PartialSolution &schedule,
                 const problem::Operation &eligibleOperation,
                 const problem::Operation &nextOperation,
                 const cli::CLIArgs &args);

/**
 * @brief Inserts a maintenance action into the schedule.
 *
 * @param problemInstance The problem instance.
 * @param machine The machine ID.
 * @param dg The delay graph.
 * @param schedule The schedule.
 * @param ASAPST The ASAPST vector.
 * @param i The index.
 * @param actionID The action ID.
 * @return A pair containing the new partial solution and the updated delay graph.
 */
std::pair<PartialSolution, cg::ConstraintGraph>
insertMaintenance(problem::Instance &problemInstance,
                  problem::MachineId machine,
                  cg::ConstraintGraph &dg,
                  const PartialSolution &schedule,
                  const std::vector<delay> &ASAPST,
                  std::ptrdiff_t i,
                  unsigned int actionID);

/**
 * @brief Fetches the idle time.
 *
 * @param problemInstance The problem instance.
 * @param machine The machine ID.
 * @param dg The delay graph.
 * @param schedule The schedule.
 * @param ASAPST The ASAPST vector.
 * @param TLU The TLU vector.
 * @param i The index.
 * @return A pair representing the idle time and the maximum idle time.
 */
std::pair<delay, delay> fetchIdle(const problem::Instance &problemInstance,
                                  problem::MachineId machine,
                                  const cg::ConstraintGraph &dg,
                                  const PartialSolution &schedule,
                                  const std::vector<delay> &ASAPST,
                                  std::vector<delay> &TLU,
                                  std::ptrdiff_t i);

/**
 * @brief Checks the interval.
 *
 * @param idle The idle time.
 * @param maintPolicy The maintenance policy.
 * @param args The command line arguments.
 * @return The interval as an unsigned integer.
 */
unsigned int checkInterval(std::pair<delay, delay> idle,
                           const problem::MaintenancePolicy &maintPolicy,
                           const cli::CLIArgs &args);

/**
 * @brief Recomputes the schedule.
 *
 * @param problemInstance The problem instance.
 * @param schedule The schedule.
 * @param maintPolicy The maintenance policy.
 * @param dg The delay graph.
 * @param InputEdges The input edges.
 * @param ASAPST The ASAPST vector.
 * @param sources The sources.
 * @param window The window.
 * @return The longest path result.
 */
algorithms::paths::LongestPathResult
recomputeSchedule(const problem::Instance &problemInstance,
                  PartialSolution &schedule,
                  const problem::MaintenancePolicy &maintPolicy,
                  cg::ConstraintGraph &dg,
                  const Sequence &inputSequence,
                  algorithms::paths::PathTimes &ASAPST,
                  const cg::VerticesCRef &sources,
                  const cg::VerticesCRef &window);
} // namespace fms::solvers::maintenance

#endif // FMS_SOLVERS_MAINTENANCE_HEURISTIC_HPP