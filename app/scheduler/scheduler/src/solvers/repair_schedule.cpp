/*
 * @author Eghonghon Eigbe
 * schedule repair in a computed schedule
 */
#include "fms/pch/containers.hpp"

#include "fms/solvers/repair_schedule.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/vertex.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/operation.hpp"
#include "fms/solvers/maintenance_heuristic.hpp"

using namespace fms;
using namespace fms::solvers;

std::tuple<PartialSolution, cg::ConstraintGraph>
RepairSchedule::repairScheduleOffline(const problem::Instance &problemInstance,
                                      cg::ConstraintGraph &dg,
                                      PartialSolution solution,
                                      problem::Operation eligibleOperation,
                                      std::vector<delay> &ASAPST) {
    const auto &maintPolicy = problemInstance.maintenancePolicy();
    problem::MachineId machine = problemInstance.getMachine(eligibleOperation);
    const problem::OperationId &firstReEntrantOp =
            problemInstance.getMachineOperations(machine).front();
    const problem::OperationId &secondReEntrantOp = firstReEntrantOp + 1;

    LOG("This schedule became infeasible because of operation {} the preceding edge is {}\n.",
        *solution.latestOp(machine),
        *(--solution.latestOp(machine)));

    problem::JobId lastFirstPass = problem::JobId::max();
    problem::JobId lastCommittedSecondPass = problem::JobId::max();

    const auto &machineSequence = solution.getMachineSequence(machine);
    auto start = machineSequence.begin();

    // find last second pass before the offending operation - offending is from the last edge
    auto startRepair = findSecondToLastFirstPass(
            problemInstance,
            solution,
            machine,
            std::distance(machineSequence.begin(), solution.latestOp(machine)));
    lastFirstPass = startRepair.first;
    start = start + startRepair.second;

    // find the second to last first pass from the offending operation
    lastCommittedSecondPass = findLastCommittedSecondPass(
            problemInstance, solution, machine, std::distance(machineSequence.begin(), start));

    // if start was not updated at all, i.e. we are stuck at the beginning of the schedule
    if (start == machineSequence.begin()) {
        throw FmsSchedulerException(
                "No repair strategy can be applied. This is not possible in the Canon case");
    }

    LOG(fmt::format("Last 1st pass is {} with edge {} while last committed 2nd pass is {}.\n",
                    lastFirstPass,
                    *start,
                    lastCommittedSecondPass));

    // perform the actual repair
    // insert second passes earlier
    std::vector<problem::Operation> insertions;
    for (problem::JobId i =
                 (lastCommittedSecondPass == problem::JobId::max() ? problem::JobId(0)
                                                                   : lastCommittedSecondPass + 1);
         i <= eligibleOperation.jobId;
         i++) {
        if (i <= lastFirstPass && problemInstance.getReEntrancies(i) == problem::plexity::DUPLEX) {
            // if second pass not in chosen edges add it
            insertions.push_back({i, secondReEntrantOp});
        }
    }

    std::tie(solution, dg) = insertRepair(problemInstance,
                                          dg,
                                          solution,
                                          eligibleOperation,
                                          ASAPST,
                                          insertions,
                                          std::distance(machineSequence.begin(), start));

    // remove second passes later to prevent duplication of operations
    std::vector<problem::Operation> removals = insertions;
    for (const cg::Vertex &v : dg.getMaintVertices()) {
        removals.push_back(v.operation);
    }

    const auto &repairedSequence = solution.getMachineSequence(machine);

    std::tie(solution, dg) =
            removeRepair(problemInstance,
                         dg,
                         solution,
                         eligibleOperation,
                         ASAPST,
                         removals,
                         std::distance(repairedSequence.begin(), solution.latestOp(machine)) + 1,
                         repairedSequence.size());

    // check if repair successful
    auto ASAPSTnew = algorithms::paths::initializeASAPST(dg);
    auto resultRepair = maintenance::recomputeSchedule(problemInstance,
                                                       solution,
                                                       maintPolicy,
                                                       dg,
                                                       solution.getMachineSequence(machine),
                                                       ASAPSTnew,
                                                       {},
                                                       {});
    if (!resultRepair.positiveCycle.empty()) {
        LOG("Infeasible schedule generated after repair!\n");
        std::tie(solution, dg) = repairScheduleOffline(
                problemInstance, dg, solution, eligibleOperation, ASAPSTnew); // repair recursively
    }

    // adjust first feasible edge
    const auto &adjustSequence = solution.getMachineSequence(machine);
    for (std::size_t i = 0; i < adjustSequence.size(); i++) {
        if (adjustSequence[i] == eligibleOperation) {
            solution.setFirstFeasibleEdge(machine, i + 1);
            break;
        }
    }
    solution.setASAPST(ASAPSTnew);
    solution.incrRepairCount();
    return std::make_tuple(solution, dg);
}

std::pair<problem::JobId, Sequence::difference_type>
RepairSchedule::findSecondToLastFirstPass(const problem::Instance &problemInstance,
                                          const PartialSolution &solution,
                                          problem::MachineId machine,
                                          Sequence::difference_type start) {
    problem::JobId lastFirstPass = problem::JobId::max();
    const problem::OperationId &firstReEntrantOp =
            problemInstance.getMachineOperations(machine).front();
    const problem::OperationId &secondReEntrantOp = firstReEntrantOp + 1;

    const auto &chosenSequence = solution.getMachineSequence(machine);

    auto startRepair = chosenSequence.begin();

    // loop back from the last insertion, this was the culprit to become infeasible
    auto looper = chosenSequence.begin() + start;

    bool updateFirst = true;
    bool updateSecond = false;
    unsigned int nrFirst = 0;
    for (; looper != chosenSequence.begin(); --looper) {
        if (looper->operationId == firstReEntrantOp && !looper->isMaintenance()) {
            nrFirst++;
            lastFirstPass = looper->jobId;
            if (nrFirst == 2) { // because we're always looking for the second earliest first pass
                updateSecond = true;
                updateFirst = false;
                startRepair = looper;
                ++startRepair;
                break;
            }
        }
    }
    return {lastFirstPass, std::distance(chosenSequence.begin(), startRepair)};
}

problem::JobId RepairSchedule::findLastCommittedSecondPass(const problem::Instance &problemInstance,
                                                           const PartialSolution &solution,
                                                           problem::MachineId machine,
                                                           std::ptrdiff_t start) {
    problem::JobId lastCommittedSecondPass = problem::JobId::max();

    const problem::OperationId &firstReEntrantOp =
            problemInstance.getMachineOperations(machine).front();
    const problem::OperationId &secondReEntrantOp = firstReEntrantOp + 1;

    auto looper = solution.getMachineSequence(machine).begin()
                  + std::max(start - 1, static_cast<std::ptrdiff_t>(0));
    for (; looper != solution.getMachineSequence(machine).begin(); --looper) {
        if (looper->operationId == secondReEntrantOp) {
            lastCommittedSecondPass = looper->jobId;
            break;
        }
    }
    return lastCommittedSecondPass;
}

std::tuple<PartialSolution, cg::ConstraintGraph>
RepairSchedule::insertRepair(const problem::Instance &problemInstance,
                             cg::ConstraintGraph &dg,
                             PartialSolution solution,
                             problem::Operation eligibleOperation,
                             std::vector<delay> &ASAPST,
                             const std::vector<problem::Operation> &ops,
                             cg::Edges::difference_type start) {
    const auto &maintPolicy = problemInstance.maintenancePolicy();
    problem::MachineId machine = problemInstance.getMachine(eligibleOperation);

    auto iter = solution.getMachineSequence(machine).begin() + start;

    for (auto op : ops) {
        LOG(fmt::format("Adding second pass for operation {}\n", op));
        auto iterE = iter;
        auto afterE = iter;
        ++afterE;

        const auto prevO = *(iterE - 1);
        const auto curO = op;
        const auto nextO = *iterE;

        SchedulingOption loopOpt(prevO,
                                 curO,
                                 nextO,
                                 std::distance(solution.getMachineSequence(machine).begin(), iter),
                                 false);

        LOG(fmt::format("Adding {} between {} and {}.\n", curO, prevO, nextO));

        // add the edges from the options to the list
        solution = solution.add(machine, loopOpt, ASAPST);
        iter = solution.latestOp(machine);
    }
    return std::make_tuple(solution, dg);
}

std::tuple<PartialSolution, cg::ConstraintGraph>
RepairSchedule::removeRepair(const problem::Instance &problemInstance,
                             cg::ConstraintGraph &dg,
                             PartialSolution solution,
                             problem::Operation eligibleOperation,
                             std::vector<delay> &ASAPST,
                             std::vector<problem::Operation> ops,
                             std::size_t start,
                             std::size_t end,
                             bool afterLast) {

    const auto &maintPolicy = problemInstance.maintenancePolicy();
    problem::MachineId machine = problemInstance.getMachine(eligibleOperation);

    auto i = start;
    auto j = start;
    while (j != end) {

        const auto &sequence = solution.getMachineSequence(machine);
        if (std::find(ops.begin(), ops.end(), sequence.at(i - 1)) != ops.end()) {

            LOG("Removing second pass for operation at {} {}\n", i, sequence[i - 1]);

            const auto prevO = sequence.at(i - 2);
            const auto curO = sequence.at(i - 1);
            const auto nextO = sequence.at(i);

            SchedulingOption remOpt(prevO, curO, nextO, i - 1);
            solution = solution.remove(machine, remOpt, ASAPST, afterLast);
            LOG(fmt::format("Removed {} before {}.\n", curO, nextO));

            if (curO.isMaintenance()) {
                solution.setMaintCount(solution.getMaintCount() - 1);
            }
        } else {
            i++;
        }
        j++;
    }
    return std::make_tuple(solution, dg);
}
