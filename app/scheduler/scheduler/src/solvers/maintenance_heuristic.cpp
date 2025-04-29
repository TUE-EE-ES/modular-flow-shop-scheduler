#include "fms/pch/containers.hpp"

#include "fms/solvers/maintenance_heuristic.hpp"

#include "fms/cg/constraint_graph.hpp"
#include "fms/problem/indices.hpp"
#include "fms/solvers/repair_schedule.hpp"

namespace fms::solvers::maintenance {

/*
In this version, maintenance operations are added with the assumption that maintenance and setup
time cannot overlap. To math the exact models, this should be changed.
*/

std::tuple<PartialSolution, cg::ConstraintGraph>
triggerMaintenance(cg::ConstraintGraph dg,
                   problem::Instance &problemInstance,
                   problem::MachineId machine,
                   const PartialSolution &solution,
                   const cli::CLIArgs &args) {
    const auto &sequence = solution.getMachineSequence(machine);
    problem::Operation nextOperation = sequence.back();
    return triggerMaintenance(
            std::move(dg), problemInstance, solution, nextOperation, nextOperation, args);
}

std::tuple<PartialSolution, cg::ConstraintGraph>
triggerMaintenance(cg::ConstraintGraph dg,
                   problem::Instance &problemInstance,
                   const PartialSolution &solution,
                   const SchedulingOption &eligibleOption,
                   const cli::CLIArgs &args) {
    problem::Operation eligibleOperation = eligibleOption.curO;
    problem::Operation nextOperation = eligibleOption.nextO;
    return triggerMaintenance(
            std::move(dg), problemInstance, solution, eligibleOperation, nextOperation, args);
}

std::tuple<PartialSolution, cg::ConstraintGraph>
triggerMaintenance(cg::ConstraintGraph dg,
                   problem::Instance &problemInstance,
                   const PartialSolution &solution,
                   const problem::Operation eligibleOperation,
                   const problem::Operation nextOperation,
                   const cli::CLIArgs &args) {
    problem::MachineId reEntrantMachineId = problemInstance.getMachine(eligibleOperation);
    // iteratively evaluate the solution until no more maintenance needs to be added
    PartialSolution oldSolution = solution;
    auto [updatedSolution, updatedDg] = evaluateSchedule(
            problemInstance, dg, oldSolution, eligibleOperation, nextOperation, args);
    while (updatedSolution.getMachineSequence(reEntrantMachineId)
           != oldSolution.getMachineSequence(reEntrantMachineId)) {
        oldSolution = updatedSolution;
        std::tie(updatedSolution, updatedDg) = evaluateSchedule(
                problemInstance, updatedDg, oldSolution, eligibleOperation, nextOperation, args);
    }
    return {updatedSolution, updatedDg};
}

std::tuple<PartialSolution, cg::ConstraintGraph>
evaluateSchedule(problem::Instance &problemInstance,
                 cg::ConstraintGraph &dg,
                 const PartialSolution &schedule,
                 const problem::Operation &eligibleOperation,
                 const problem::Operation &nextOperation,
                 const cli::CLIArgs &args) {

    auto ASAPST = schedule.getASAPST();
    const auto &maintPolicy = problemInstance.maintenancePolicy();
    problem::MachineId machine = problemInstance.getMachine(eligibleOperation);
    std::optional<problem::JobId> lastCommittedSecondPass;

    const auto totalSizes = problemInstance.getMaximumSheetSize();
    std::vector<delay> TLU(totalSizes + 1, 0);

    const auto &sequence = schedule.getMachineSequence(machine);

    std::ptrdiff_t i = std::distance(sequence.begin(), schedule.firstMaintOp(machine));
    std::optional<problem::Operation> prevOp;

    for (; i < sequence.size() && prevOp != eligibleOperation; i++) {
        // compute idle time trying to consider sheet properties
        auto fetch = fetchIdle(problemInstance, machine, dg, schedule, ASAPST, TLU, i);

        if (prevOp.has_value() && prevOp->operationId == 2) {
            lastCommittedSecondPass = prevOp->jobId;
        }

        // add operation to the delay graph and add necessary edges both to delay graph and
        // schedule
        unsigned int actionID = checkInterval(fetch, maintPolicy, args);
        if (actionID != problem::Instance::MAINT_ID.value) {
            LOG("Maintenance triggered after op {}", sequence.at(i));
            // add the edges from the options to the list
            auto q = insertMaintenance(problemInstance, machine, dg, schedule, ASAPST, i, actionID);
            auto new_solution = q.first;
            dg = q.second;
            new_solution.incrMaintCount();

            ASAPST.push_back(std::numeric_limits<delay>::min());

            cg::VerticesCRef sources;
            if (lastCommittedSecondPass.has_value()) {
                sources = dg.getVerticesC(*lastCommittedSecondPass);
            } else {
                sources = cg::VerticesCRef{dg.getVertex(problem::Operation{problem::JobId(0), 0})};
            }
            const auto windowStart = lastCommittedSecondPass.value_or(problem::JobId{0});
            auto window = dg.getVertices(windowStart + 1, nextOperation.jobId);
            auto m = dg.getMaintVertices();
            window.insert(window.end(), m.begin(), m.end());

            auto ASAPSTold = ASAPST; // stash old ASAPST
            auto result = recomputeSchedule(problemInstance,
                                            new_solution,
                                            maintPolicy,
                                            dg,
                                            new_solution.getMachineSequence(machine),
                                            ASAPST,
                                            sources,
                                            window);

            if (!result.positiveCycle.empty()) {
                LOG("Schedule Repair Triggered.");
                std::tie(new_solution, dg) = RepairSchedule::repairScheduleOffline(
                        problemInstance, dg, new_solution, eligibleOperation, ASAPST);
            }
            return std::make_tuple(new_solution, dg);
        }
        prevOp = sequence.at(i);
    }
    return std::make_tuple(schedule, dg);
}

std::pair<PartialSolution, cg::ConstraintGraph>
insertMaintenance(problem::Instance &problemInstance,
                  const problem::MachineId machine,
                  cg::ConstraintGraph &dg,
                  const PartialSolution &schedule,
                  const std::vector<delay> &ASAPST,
                  const std::ptrdiff_t i,
                  unsigned int actionID) {

    const problem::OperationId &firstReEntrantOp =
            problemInstance.getMachineOperations(machine).front();
    const auto &sequence = schedule.getMachineSequence(machine);

    const auto op = problemInstance.addMaintenanceOperation(actionID);
    cg::VertexId maint = dg.addVertex(op); // create maintenance vertex

    const auto prevO = sequence[i - 1];
    const auto currO = dg.getVertex(maint).operation;
    const auto nextO = sequence[i];

    SchedulingOption maint_opt(
            prevO, currO, nextO, std::distance(sequence.begin(), sequence.begin() + i), true);
    // add the edges from the options to the list
    auto new_solution = schedule.add(machine, maint_opt, ASAPST);
    return {std::move(new_solution), dg};
}

std::pair<delay, delay> fetchIdle(const problem::Instance &problemInstance,
                                  const problem::MachineId machine,
                                  const cg::ConstraintGraph &dg,
                                  const PartialSolution &schedule,
                                  const std::vector<delay> &ASAPST,
                                  std::vector<delay> &TLU,
                                  const std::ptrdiff_t i) {
    const auto &maintPolicy = problemInstance.maintenancePolicy();
    const auto totalSizes = problemInstance.getMaximumSheetSize();
    const auto &sequence = schedule.getMachineSequence(machine);
    const auto &currO = sequence.at(i);
    const auto &currV = dg.getVertex(currO);

    if (i <= 0) {
        std::ranges::fill(TLU, 0);
    } else {
        const auto &prevO = sequence.at(i - 1);
        const auto &prevV = dg.getVertex(prevO);

        if (currO.isMaintenance()) { // maintenance resets everybody
            std::ranges::fill(TLU, 0);
        } else if (prevO.isMaintenance()) {
            const auto newTLU = ASAPST[currV.id] - ASAPST[prevV.id]
                                - maintPolicy.getMaintDuration(prevO.maintId.value());
            std::ranges::fill(TLU, newTLU);
        }
        // reset TLU of this sheet size and all smaller sheet sizes to 0 and increment TLU of all
        // bigger sizes
        else {
            const auto prevSize = problemInstance.getSheetSize(prevO);
            const auto newTLU =
                    ASAPST[currV.id] - ASAPST[prevV.id] - problemInstance.getProcessingTime(prevO);
            std::fill_n(TLU.begin(), prevSize + 1, newTLU);

            for (unsigned int j = prevSize + 1; j <= totalSizes; ++j) {
                TLU[j] += ASAPST[currV.id] - ASAPST[prevV.id];
            }
        }
    }

    // check if maintenance should be triggered before the destination sheet use size of
    // destination sheet to choose
    delay idle{};
    delay maxidle{};
    if (currO.isMaintenance()) {
        idle = TLU[0];
    } else {
        auto uniqueSheetSizes = problemInstance.getUniqueSheetSizes();
        for (unsigned int j = 0; j <= totalSizes; ++j) {
            if ((uniqueSheetSizes.find(j) != uniqueSheetSizes.end()) && TLU[j] > maxidle) {
                maxidle = TLU[j];
            }
        }
        idle = TLU[problemInstance.getSheetSize(sequence[i])];
    }

    return {idle, maxidle};
}

unsigned int checkInterval(std::pair<delay, delay> idle,
                           const problem::MaintenancePolicy &maintPolicy,
                           const cli::CLIArgs &args) {
    // Gets an interval and the policy returns id of triggered action
    auto interval = idle.first;
    auto maxinterval = idle.second;

    constexpr const double maxintervalThreshold = 0.9;

    for (unsigned int i = 0; i < maintPolicy.getNumberOfTypes(); i++) {
        const auto &[minV, maxV] = maintPolicy.getThresholds(i);

        switch (args.algorithm) {
        case cli::AlgorithmType::MIBHCS:
        case cli::AlgorithmType::MINEH:
        case cli::AlgorithmType::MIASAP:
            if (((interval >= minV) && (interval < maxV))
                || ((static_cast<double>(maxinterval)
                     >= maxintervalThreshold * static_cast<double>(maxV))
                    && (maxinterval < maxV))) {
                return i;
            }
            break;
        case cli::AlgorithmType::MISIM:
        case cli::AlgorithmType::MINEHSIM:
        case cli::AlgorithmType::MIASAPSIM:
            if (((interval >= minV) && (interval < maxV))) {
                return i;
            }
            break;
        default:
            throw FmsSchedulerException(std::string()
                                        + "Algorithm not recognised for maintenance insertion.");
        }
    }
    return problem::Instance::MAINT_ID.value;
}

algorithms::paths::LongestPathResult
recomputeSchedule(const problem::Instance &problemInstance,
                  PartialSolution &schedule,
                  const problem::MaintenancePolicy &maintPolicy,
                  cg::ConstraintGraph &dg,
                  const Sequence &inputSequence,
                  std::vector<delay> &ASAPST,
                  const cg::VerticesCRef &sources,
                  const cg::VerticesCRef &window) {
    problem::MachineId machine = problemInstance.getMachine(inputSequence[0]);
    std::vector<cg::Edge> addedEdges;

    std::reference_wrapper<cg::Vertex> previous = dg.getSource(machine);
    for (std::size_t i = 0; i < inputSequence.size(); i++) {
        const auto &op = inputSequence[i];
        auto &v = dg.getVertex(op);

        if (!dg.hasEdge(previous, v)) {
            delay weight{};
            if (op.isMaintenance()) {
                // Adding a maintenance operation extends the duration between the previous
                // and the next operation by the duration of the maintenance operation.
                // Thus, we still need to fetch the time of the normal operation (i.e., without
                // maintenance). This also preserves the sequence-dependent setup times.
                // Ideally, this process should happen inside the "query" function but this function
                // doesn't have a way to know the next operation in the sequence.
                weight = problemInstance.query(previous, dg.getVertex(inputSequence.at(i + 1)));
            } else {
                weight = problemInstance.query(previous, v);
            }
            cg::Edge e = dg.addEdge(previous.get(), v, weight);
            addedEdges.push_back(e);
        }

        if (previous.get().operation.isMaintenance()) {
            delay dueWeight = maintPolicy.getMaintDuration(previous.get().operation)
                              + maintPolicy.getMinimumIdle() - 1;
            cg::Edge dueEdge = dg.addEdge(v, previous, -dueWeight);
            addedEdges.push_back(dueEdge);
        }

        previous = v;
    }

    algorithms::paths::LongestPathResult result;

    if (window.empty()) {
        result = algorithms::paths::computeASAPST(dg, ASAPST);
    } else {
        result = algorithms::paths::computeASAPST(dg, ASAPST, sources, window);
    }
    schedule.setASAPST(ASAPST);

    for (const auto &i : addedEdges) {
        dg.removeEdge(i);
    }
    return result;
}

} // namespace fms::solvers::maintenance
