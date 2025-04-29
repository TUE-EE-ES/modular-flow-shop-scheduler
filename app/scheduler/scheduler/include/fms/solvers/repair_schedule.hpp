#ifndef FMS_SOLVERS_REPAIR_SCHEDULE_HPP
#define FMS_SOLVERS_REPAIR_SCHEDULE_HPP

#include "fms/cg/constraint_graph.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/operation.hpp"
#include "fms/solvers/partial_solution.hpp"

#include <cstddef>
#include <vector>

namespace fms::solvers {

/**
 * @class RepairSchedule
 * @brief Class for handling repair schedules.
 *
 * This class provides methods for repairing schedules offline, inserting repairs into schedules,
 * removing repairs from schedules, finding the second to last first pass, finding the last
 * committed second pass, and recomputing schedules.
 */
class RepairSchedule {
public:
    /**
     * @brief Repairs the schedule offline.
     *
     * @param problemInstance The problem instance.
     * @param dg The delay graph.
     * @param solution The partial solution.
     * @param eligibleOperation The eligible operation.
     * @param ASAPST The ASAPST vector.
     * @return A tuple containing the new partial solution and the updated delay graph.
     */
    static std::tuple<PartialSolution, cg::ConstraintGraph>
    repairScheduleOffline(const problem::Instance &problemInstance,
                          cg::ConstraintGraph &dg,
                          PartialSolution solution,
                          problem::Operation eligibleOperation,
                          std::vector<delay> &ASAPST);

    /**
     * @brief Inserts a repair into the schedule.
     *
     * @param problemInstance The problem instance.
     * @param dg The delay graph.
     * @param solution The partial solution.
     * @param eligibleOperation The eligible operation.
     * @param ASAPST The ASAPST vector.
     * @param ops The operations vector.
     * @param start The start index.
     * @return A tuple containing the new partial solution and the updated delay graph.
     */
    static std::tuple<PartialSolution, cg::ConstraintGraph>
    insertRepair(const problem::Instance &problemInstance,
                 cg::ConstraintGraph &dg,
                 PartialSolution solution,
                 problem::Operation eligibleOperation,
                 std::vector<delay> &ASAPST,
                 const std::vector<problem::Operation> &ops,
                 cg::Edges::difference_type start);

    /**
     * @brief Removes a repair from the schedule.
     *
     * @param problemInstance The problem instance.
     * @param dg The delay graph.
     * @param solution The partial solution.
     * @param eligibleOperation The eligible operation.
     * @param ASAPST The ASAPST vector.
     * @param ops The operations vector.
     * @param start The start index.
     * @param end The end index.
     * @param afterLast A boolean indicating whether to remove after the last operation.
     * @return A tuple containing the new partial solution and the updated delay graph.
     */
    static std::tuple<PartialSolution, cg::ConstraintGraph>
    removeRepair(const problem::Instance &problemInstance,
                 cg::ConstraintGraph &dg,
                 PartialSolution solution,
                 problem::Operation eligibleOperation,
                 std::vector<delay> &ASAPST,
                 std::vector<problem::Operation> ops,
                 std::size_t start,
                 std::size_t end,
                 bool afterLast = true);

    /**
     * @brief Finds the second to last first pass.
     *
     * @param problemInstance The problem instance.
     * @param dg The delay graph.
     * @param solution The partial solution.
     * @param machine The machine ID.
     * @param start The start index.
     * @return A pair containing the job ID and the difference type.
     */
    static std::pair<problem::JobId, Sequence::difference_type>
    findSecondToLastFirstPass(const problem::Instance &problemInstance,
                              const PartialSolution &solution,
                              problem::MachineId machine,
                              Sequence::difference_type start);

    /**
     * @brief Finds the last committed second pass.
     *
     * @param problemInstance The problem instance.
     * @param solution The partial solution.
     * @param machine The machine ID.
     * @param start The start index.
     * @return The job ID.
     */
    static problem::JobId findLastCommittedSecondPass(const problem::Instance &problemInstance,
                                                      const PartialSolution &solution,
                                                      problem::MachineId machine,
                                                      std::ptrdiff_t start);
};
} // namespace fms::solvers

#endif // FMS_SOLVERS_REPAIR_SCHEDULE_HPP