#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/solvers/forward_heuristic.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/export_utilities.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/solvers/maintenance_heuristic.hpp"
#include "fms/solvers/utils.hpp"

#include <chrono>

namespace fms::solvers::forward {

PartialSolution solve(problem::Instance &problemInstance, const cli::CLIArgs &args) {
    // solve the instance
    LOG("Computation of the schedule started");

    auto ASAPST = SolversUtils::initProblemGraph(problemInstance, IS_LOG_D());
    auto dg = problemInstance.getDelayGraph();
    LOG("Number of vertices in the delay graph is {}", dg.getNumberOfVertices());

    // We only support a single re-entrant machine in the system so choose the first one
    problem::MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();
    if (problemInstance.getMachineOperations(reentrant_machine).size() > 2) {
        throw std::runtime_error("Multiple re-entrancies not implemented yet");
    }

    Sequence initialSequence = createInitialSequence(problemInstance, reentrant_machine);
    PartialSolution solution({{reentrant_machine, initialSequence}}, ASAPST);

    const auto &ops = problemInstance.getMachineOperations(reentrant_machine);
    const auto &jobs = problemInstance.getJobsOutput();

    // iteratively schedule eligible nodes (insert higher passes between the existing sequence)
    for (std::size_t i = 0; i + 1 < jobs.size(); ++i) {
        const auto &jobOps = problemInstance.getJobOperationsOnMachine(jobs[i], reentrant_machine);

        // First operation is already included in the initial sequence
        for (std::size_t i = 1; i < jobOps.size(); ++i) {
            solution = scheduleOneOperation(dg, problemInstance, solution, jobOps.at(i), args);
        }
    }

    switch (args.algorithm) {
    case cli::AlgorithmType::MIBHCS:
    case cli::AlgorithmType::MISIM:
    case cli::AlgorithmType::MIASAP:
    case cli::AlgorithmType::MIASAPSIM:
        std::tie(solution, dg) = maintenance::triggerMaintenance(
                dg, problemInstance, reentrant_machine, solution, args);
        problemInstance.updateDelayGraph(dg);
        break;
    default:
        break;
    }
    solution.addInferredInputSequence(problemInstance);

    if (IS_LOG_D()) {
        auto name = fmt::format("output_graph_bhcs_{}.dot", problemInstance.getProblemName());
        cg::exports::saveAsDot(problemInstance, solution, name);
    }

    return solution;
}

Sequence createInitialSequence(const problem::Instance &problemInstance,
                               problem::MachineId reEntrantMachine) {
    Sequence initialSequence;
    const cg::ConstraintGraph &dg = problemInstance.getDelayGraph();
    problem::ReEntrantId reEntrantMachineId =
            problemInstance.findMachineReEntrantId(reEntrantMachine);

    // check how many operations are mapped
    const auto &ops = problemInstance.getMachineOperations(reEntrantMachine);

    /// no ordering is required if the number of operations mapped is 1
    if (ops.size() <= 1) {
        throw std::runtime_error(fmt::format("Machine {} is not re-entrant", reEntrantMachine));
    }

    std::optional<problem::JobId> lastDuplexJob;

    // Add all first passes (of _duplex_ jobs) to the initial sequence
    for (auto jobId : problemInstance.getJobsOutput()) { // For all but the last job
        if (problemInstance.getReEntrancies(jobId, reEntrantMachineId)
            == problem::plexity::DUPLEX) {
            const auto &jobOps = problemInstance.getJobOperationsOnMachine(jobId, reEntrantMachine);
            initialSequence.push_back(jobOps.front());
            lastDuplexJob = jobId;
        }
    }

    if (!lastDuplexJob.has_value()) {
        throw FmsSchedulerException("Nothing to schedule; only simplex sheets!");
    }

    // For the last duplex job add its remaining passes to the initial sequence
    const auto &jobOps =
            problemInstance.getJobOperationsOnMachine(lastDuplexJob.value(), reEntrantMachine);
    for (std::size_t i = 1; i < jobOps.size(); ++i) {
        initialSequence.push_back(jobOps.at(i));
    }

    return initialSequence;
}

std::pair<problem::Operation, std::vector<SchedulingOption>>
createOptions(const problem::Instance &problem,
              const PartialSolution &solution,
              const cg::Vertex &eligibleOperation,
              problem::MachineId reEntrantMachineId) {
    const auto &currentSequence = solution.getMachineSequence(reEntrantMachineId);
    auto lastPotentiallyFeasibleOption = currentSequence.back();

    if (IS_LOG_I()) {
        // Converting the edges to string is very expensive so we only do it if the log level is
        // the appropriate level
        LOG(chosenSequencesToString(solution));
    }

    // create a set of potentially feasible options (i.e. replace an edge by two edges),
    // and check whether they are feasible as new partial solutions
    std::vector<SchedulingOption> options;
    delay totalOperationTime = 0;

    // we cannot schedule before the last inserted edge
    const delay currentDeadline = determineSmallestDeadline(eligibleOperation);
    const auto firstPossibleOp = solution.firstPossibleOp(reEntrantMachineId);

    for (auto pOp = firstPossibleOp; pOp != currentSequence.end(); pOp++) {
        if (pOp == currentSequence.begin()) {
            // We cannot insert before the first operation. This would imply that we are doing the
            // second pass of the first operation before the first pass.
            continue;
        }

        lastPotentiallyFeasibleOption = *pOp;

        const auto prevOp = *(pOp - 1);
        const auto curOp = eligibleOperation.operation;
        const auto nextOp = *pOp;

        // keep track of the previous and next vertex

        const auto &curV = eligibleOperation;
        const delay prevNextWeight = problem.query(prevOp, nextOp);

        // not allowed to create an option in case of a flush!
        if (curOp.jobId != nextOp.jobId) {

            // avoid inconsistent total ordering (this should be an invariant in the creation of
            // options...)
            auto distance = std::distance(currentSequence.begin(), pOp);
            LOG(FMT_COMPILE("Creating option {}->{}->{}: {}"), prevOp, curOp, nextOp, distance);
            SchedulingOption c(prevOp, curOp, nextOp, distance);

            const delay prevCurWeight = problem.query(prevOp, curOp);
            const delay curNextWeight = problem.query(curOp, nextOp);

            if (prevOp.jobId != nextOp.jobId
                && problem.query(prevOp, nextOp) > prevCurWeight + curNextWeight) {
                LOG_W("Triangle inequality violated! {} -> {} = {} > {} -> {} -> {} = {}",
                      prevOp,
                      nextOp,
                      prevNextWeight,
                      prevOp,
                      curOp,
                      nextOp,
                      prevCurWeight + curNextWeight);
            }
            options.push_back(c);
        }

        if (totalOperationTime > currentDeadline) {
            // As the currentDeadline would have been expired if we would schedule the operation
            // here, we can stop searching for options: any schedule must be infeasible.

            // cannot become feasible anymore; stop adding interleaving options to this partial
            // solution
            break;
        }
        totalOperationTime += prevNextWeight;
    }

    return {lastPotentiallyFeasibleOption, options};
}

std::optional<std::pair<PartialSolution, SchedulingOption>>
evaluateOptionFeasibility(cg::ConstraintGraph &dg,
                          const problem::Instance &problem,
                          const PartialSolution &solution,
                          const SchedulingOption &options,
                          const std::vector<delay> &ASAPTimes,
                          problem::MachineId reEntrantMachine) {
    auto newGenerationOfSolutions = evaluateOptionFeasibility(
            dg, problem, solution, std::vector{options}, ASAPTimes, reEntrantMachine);
    if (!newGenerationOfSolutions.empty()) {
        return newGenerationOfSolutions.front();
    }
    return std::nullopt;
}

std::vector<std::pair<PartialSolution, SchedulingOption>>
evaluateOptionFeasibility(cg::ConstraintGraph &dg,
                          const problem::Instance &problem,
                          const PartialSolution &solution,
                          const std::vector<SchedulingOption> &options,
                          const std::vector<delay> &ASAPTimes,
                          problem::MachineId reEntrantMachine) {
    unsigned int nrFeasibleOptions = 0;
    unsigned int nrInfeasibleOptions = 0;

    const auto firstJobId = problem.getJobsOutput().front();
    const auto firstOp = problem.jobs(firstJobId).front();

    std::vector<std::pair<PartialSolution, SchedulingOption>> newGenerationOfSolutions;
    for (const SchedulingOption &o : options) {
        // create a local copy that we can modify without issues
        std::vector<delay> ASAPST = ASAPTimes;

        // Add the edges from the options to the list. ASAPTimes are not (yet) valid for the updated
        // solution... but we will use them only for the chosen edges.
        PartialSolution ps = solution.add(reEntrantMachine, o, ASAPTimes);

        // make a copy of the chosen edges
        cg::Edges final_sequence = ps.getAllAndInferredEdges(problem);

        const auto &curV = dg.getVertex(o.curO);
        const auto &nextV = dg.getVertex(o.nextO);

        LOG_D(FMT_COMPILE("Checking feasibility of interleaving {} between {} and "
                          "{}"),
              o.curO,
              o.prevO,
              o.nextO);
        const problem::JobId jobStart = o.curO.jobId;

        cg::VerticesCRef origin{dg.getVertex(firstOp)};
        cg::VerticesCRef sourcevertices =
                (jobStart == firstOp.jobId)
                        ? origin
                        : dg.getVerticesC(std::max(jobStart, problem::JobId(1)) - 1);
        cg::VerticesCRef windowvertices = dg.getVerticesC(jobStart, o.nextO.jobId);

        auto m = dg.getMaintVertices();
        windowvertices.insert(windowvertices.end(), m.begin(), m.end());

        auto result = validateInterleaving(
                dg, problem, final_sequence, ASAPST, sourcevertices, windowvertices);

        delay interleaved_starting_time = ASAPST[curV.id];

        if (result.positiveCycle.empty()) {
            PartialSolution p_sol = solution.add(reEntrantMachine, o, ASAPST);
            p_sol.setMakespanLastScheduledJob(interleaved_starting_time);

            // set the (relaxed) starting time of the interleaved operation and remaining
            // flexibility
            auto [avgProd, nrJobs] =
                    computeFutureAvgProductivy(dg, ASAPST, p_sol, reEntrantMachine);

            p_sol.setAverageProductivity(avgProd / nrJobs);
            p_sol.setNrOpsInLoop(nrJobs);
            p_sol.setEarliestStartFutureOperation(ASAPST[nextV.id]);

            newGenerationOfSolutions.emplace_back(p_sol, o);
            nrFeasibleOptions++;
        } else {
            LOG_D(FMT_COMPILE("Skipping infeasible option {}->{}->{} with partial makespan "),
                  o.prevO,
                  o.curO,
                  o.nextO,
                  interleaved_starting_time);
            nrInfeasibleOptions++;
        }
    }
    LOG_D(FMT_COMPILE("Infeasible: {}"), nrInfeasibleOptions);
    return newGenerationOfSolutions;
}

delay determineSmallestDeadline(const cg::Vertex &v) {
    delay currentDeadline =
            std::numeric_limits<delay>::max(); // equivalent to the maximal permissible deadline
    // look for the smallest incoming edge with negative value (i.e. outgoing deadline)
    for (const auto &[_, weight] : v.getOutgoingEdges()) {
        if (weight < 0) {
            currentDeadline = std::min(currentDeadline, -weight);
        }
    }
    return currentDeadline;
}

PartialSolution scheduleOneOperation(cg::ConstraintGraph &dg,
                                     problem::Instance &problem,
                                     const PartialSolution &solution,
                                     const problem::Operation &eligibleOperation,
                                     const cli::CLIArgs &args) {
    using a_clock = std::chrono::steady_clock;
    a_clock::time_point start = a_clock::now();

    LOG_D(FMT_COMPILE("Starting from current solution: {}"), solution);

    auto reEntrantMachineId = problem.getMachine(eligibleOperation);
    auto [solutions, minSolId] =
            getFeasibleOptions(dg, problem, dg.getVertex(eligibleOperation), solution, args);

    LOG_D(FMT_COMPILE("*** nr option: {}"), solutions.size());

    if (!minSolId.has_value()) { // none of the solutions were feasible...
        const auto allEdges = solution.getAllChosenEdges(problem);
        // create a local copy that we can modify without issues
        auto result = algorithms::paths::getPositiveCycle(dg, allEdges);
        cg::exports::saveAsDot(problem,
                               solution,
                               fmt::format("infeasible_{}.dot", problem.getProblemName()),
                               result);

        throw FmsSchedulerException(
                fmt::format("No feasible option has been detected for operation {}. This is not "
                            "possible in the Canon case",
                            eligibleOperation));
    }

    auto &[bestSolution, newDg] = solutions[minSolId.value()];

    if (newDg) {
        dg = std::move(*newDg);
        problem.updateDelayGraph(dg);
    }
    a_clock::time_point end = a_clock::now();

    LOG(fmt::format(FMT_COMPILE("Scheduled operation {} after operation {} in {} ms."),
                    eligibleOperation,
                    *(bestSolution.firstPossibleOp(reEntrantMachineId) - 1),
                    std::chrono::duration_cast<std::chrono::milliseconds>(end - start)));
    return bestSolution;
}

std::pair<delay, unsigned int>
computeFutureAvgProductivy(cg::ConstraintGraph &dg,
                           const std::vector<delay> &ASAPST,
                           const PartialSolution &ps,
                           const problem::MachineId reEntrantMachineId) {

    const auto itFirst = ps.firstPossibleOp(reEntrantMachineId);
    const auto &nextO = *itFirst;
    const auto &curO = *(itFirst - 1);

    const auto nrOps = countOpsInBuffer(ps, reEntrantMachineId);

    delay used_buffer_time = 0;
    problem::Operation op1{nextO.jobId, curO.operationId};
    problem::Operation op2{curO.jobId, curO.operationId - 1};
    if (dg.hasVertex(op1) && dg.hasVertex(op2)) {
        used_buffer_time = ASAPST[dg.getVertexId(op1)] - ASAPST[dg.getVertexId(op2)];
    }

    return {used_buffer_time, nrOps};
}

algorithms::paths::LongestPathResult validateInterleaving(cg::ConstraintGraph &dg,
                                                          const problem::Instance &problem,
                                                          const cg::Edges &inputEdges,
                                                          std::vector<delay> &ASAPST,
                                                          const cg::VerticesCRef &sources,
                                                          const cg::VerticesCRef &window) {
    const auto &maintPolicy = problem.maintenancePolicy();
    cg::Edges edges;
    // insert the edges to the graph
    for (const auto &i : inputEdges) {
        if (!dg.hasEdge(i.src, i.dst)) {
            dg.addEdges(i);
            edges.emplace_back(i);
        }
        if (dg.getOperation(i.src).isMaintenance()) {
            delay dueWeight = maintPolicy.getMaintDuration((dg.getVertex(i.src)).operation)
                              + maintPolicy.getMinimumIdle() - 1;
            edges.emplace_back(dg.addEdge(i.dst, i.src, -dueWeight));
        }
    }

    // Compute the updated ASAP times and check the bounds
    auto result = algorithms::paths::computeASAPST(dg, ASAPST, sources, window);

    for (const auto &i : edges) {
        dg.removeEdge(i);
    }

    return result;
}
std::optional<std::size_t> rankSolutionsASAP(
        std::vector<
                std::tuple<PartialSolution, SchedulingOption, std::shared_ptr<cg::ConstraintGraph>>>
                &solutions) {
    // rank all options, normalized
    delay minStart = std::numeric_limits<delay>::max();

    // select the solution with the lowest rank
    std::optional<std::size_t> minRankId;

    for (std::size_t i = 0; i < solutions.size(); ++i) {
        auto &[sol, c, mdg] = solutions[i];
        const auto &ASAPST = sol.getASAPST();
        delay start = std::numeric_limits<delay>::max();

        start = ASAPST[mdg->getVertexId(c.curO)];

        // select the solution with minimum slack:
        if (start <= minStart) {
            minStart = start;
            minRankId = i;
            // dg = mdg; // update delay graph to chosen
        }
    }

    return minRankId;
}

std::optional<std::size_t> rankSolutions(
        std::vector<
                std::tuple<PartialSolution, SchedulingOption, std::shared_ptr<cg::ConstraintGraph>>>
                &solutions,
        algorithms::paths::PathTimes &ASAPTimes,
        problem::MachineId reEntrantMachine,
        const cli::CLIArgs &args) {
    // rank all options, normalized
    delay minPush = std::numeric_limits<delay>::max();
    delay maxPush = std::numeric_limits<delay>::min();
    delay minPushNext = std::numeric_limits<delay>::max();
    delay maxPushNext = std::numeric_limits<delay>::min();

    auto minOpsInBuffer = std::numeric_limits<uint32_t>::max();
    auto maxOpsInBuffer = std::numeric_limits<uint32_t>::min();

    for (auto &[sol, c, mdg] : solutions) {
        const auto &curVId = mdg->getVertexId(c.curO);
        const auto &nextVId = mdg->getVertexId(c.nextO);
        const auto &ASAPST = sol.getASAPST();
        const delay push = ASAPST[curVId] - ASAPTimes[curVId];
        const delay push_next = ASAPST[nextVId] - ASAPTimes[nextVId];

        const auto nrOps = countOpsInBuffer(sol, reEntrantMachine);
        sol.setNrOpsInLoop(nrOps);
        sol.setMakespanLastScheduledJob(ASAPST[curVId]);
        sol.setEarliestStartFutureOperation(push);

        minPush = std::min(minPush, push);
        maxPush = std::max(maxPush, push);

        minPushNext = std::min(minPushNext, push_next);
        maxPushNext = std::max(maxPushNext, push_next);

        minOpsInBuffer = std::min(minOpsInBuffer, nrOps);
        maxOpsInBuffer = std::max(maxOpsInBuffer, nrOps);
    }

    // select the solution with the lowest rank
    double minRank = std::numeric_limits<double>::max();
    std::optional<std::size_t> minRankId;

    for (std::size_t i = 0; i < solutions.size(); ++i) {
        auto &[sol, c, mdg] = solutions[i];
        const auto curVId = mdg->getVertexId(c.curO);
        const auto nextVId = mdg->getVertexId(c.nextO);

        const auto &ASAPST = sol.getASAPST();
        const delay push = ASAPST[curVId] - ASAPTimes[curVId];
        const delay push_next = ASAPST[nextVId] - ASAPTimes[nextVId];

        LOG_I(FMT_COMPILE("Earliest current op time: {}, earliest future op time: {}, "
                          "push_next: {}, nr ops committed "),
              ASAPST[curVId],
              ASAPST[nextVId],
              push_next,
              sol.getNrOpsInLoop());

        auto pushRange = static_cast<double>((maxPush != minPush) ? (maxPush - minPush) : 1);
        auto pushNextRange =
                static_cast<double>((maxPushNext != minPushNext) ? (maxPushNext - minPushNext) : 1);
        auto nrOpsRange = static_cast<double>(
                (maxOpsInBuffer != minOpsInBuffer) ? (maxOpsInBuffer - minOpsInBuffer) : 1);

        auto pushNorm = static_cast<double>(push - minPush) / pushRange;
        auto pushNextNorm = static_cast<double>(push_next - minPushNext) / pushNextRange;
        auto nrOpsNorm = static_cast<double>(sol.getNrOpsInLoop() - minOpsInBuffer) / nrOpsRange;

        LOG_I(FMT_COMPILE("Push (norm.): {}, push_next (norm.): {}, nrOps (norm): {}"),
              pushNorm,
              pushNextNorm,
              nrOpsNorm);

        double curRank =
                // minimize effort to come to this scheduling decision
                args.flexibilityWeight * pushNorm
                // minimize effort of executing committed work
                + args.productivityWeight * pushNextNorm
                // maximize size of committed work
                + args.tieWeight * nrOpsNorm;

        sol.setRanking(curRank);

        LOG_I(FMT_COMPILE("Rank: (norm.): {} - {}, {}"), curRank, c.prevO, c.nextO);

        // select the solution with lowest rank:
        if (curRank < minRank) {
            minRank = curRank;
            minRankId = i;
            // dg = mdg; // update delay graph to chosen
        }
    }

    return minRankId;
}

std::tuple<std::vector<std::tuple<PartialSolution, std::shared_ptr<cg::ConstraintGraph>>>,
           std::optional<std::size_t>>
getFeasibleOptions(cg::ConstraintGraph &dg,
                   problem::Instance &problem,
                   const cg::Vertex &eligibleOperation,
                   const PartialSolution &solution,
                   const cli::CLIArgs &args) {
    problem::MachineId reEntrantMachineId = problem.getMachine(eligibleOperation.operation);

    // create all options that are potentially feasible:
    auto [lastPotentiallyFeasibleOption, options] =
            createOptions(problem, solution, eligibleOperation, reEntrantMachineId);

    // update the ASAPTimes for the coming window, so that we have enough information to compute the
    // ranking
    const problem::JobId job_start = eligibleOperation.operation.jobId;
    std::vector<delay> ASAPTimes = solution.getASAPST();

    algorithms::paths::computeASAPST(
            dg,
            ASAPTimes,
            dg.getVerticesC(std::max(job_start, problem::JobId(1U)) - 1),
            dg.getVerticesC(job_start, lastPotentiallyFeasibleOption.jobId));

    std::vector<std::pair<PartialSolution, SchedulingOption>> generationOfSolutions =
            evaluateOptionFeasibility(
                    dg, problem, solution, options, ASAPTimes, reEntrantMachineId);
    std::vector<std::tuple<PartialSolution, SchedulingOption, std::shared_ptr<cg::ConstraintGraph>>>
            newGenerationOfSolutions;

    auto dgPtr = std::make_shared<cg::ConstraintGraph>(std::move(dg));
    for (auto &[sol, opt] : generationOfSolutions) {
        switch (args.algorithm) {
        case cli::AlgorithmType::MIBHCS:
        case cli::AlgorithmType::MIASAP: {
            auto [maintSolution, maintDg] =
                    maintenance::triggerMaintenance(*dgPtr, problem, sol, opt, args);
            newGenerationOfSolutions.emplace_back(
                    maintSolution, opt, std::make_shared<cg::ConstraintGraph>(std::move(maintDg)));
            break;
        }
        default: {
            newGenerationOfSolutions.emplace_back(sol, opt, dgPtr);
        }
        }
    }

    std::optional<std::size_t> minRankId;
    switch (args.algorithm) {
    case cli::AlgorithmType::ASAP:
    case cli::AlgorithmType::MIASAP:
    case cli::AlgorithmType::MIASAPSIM:
        minRankId = rankSolutionsASAP(newGenerationOfSolutions);
        break;
    default:
        minRankId = rankSolutions(newGenerationOfSolutions, ASAPTimes, reEntrantMachineId, args);
        break;
    }

    std::vector<std::tuple<PartialSolution, std::shared_ptr<cg::ConstraintGraph>>> result;
    result.reserve(newGenerationOfSolutions.size());
    for (auto &[sol, _, dg] : newGenerationOfSolutions) {
        if ((args.algorithm != cli::AlgorithmType::MIBHCS)
            && (args.algorithm != cli::AlgorithmType::MIASAP)) {
            dg.reset();
        }
        result.emplace_back(std::move(sol), dg);
    }

    dg = std::move(*dgPtr);
    dgPtr.reset();
    return {std::move(result), minRankId};
}

std::uint32_t countOpsInBuffer(const PartialSolution &ps,
                               const problem::MachineId reEntrantMachineId) {
    const auto itFirst = ps.firstPossibleOp(reEntrantMachineId);
    const auto &sequence = ps.getMachineSequence(reEntrantMachineId);
    auto iter = std::make_reverse_iterator(itFirst);
    if (std::distance(iter, sequence.rend()) < 2) {
        throw FmsSchedulerException("At least three operations should be scheduled");
    }

    const auto &curO = *iter;

    // the job's predecessor
    problem::Operation end{curO.jobId, curO.operationId - 1};

    // Start from the previous operation
    iter++;
    int nrOps = 1;
    for (; iter != sequence.rend() && *iter != end; ++iter) {
        nrOps++;
    }

    return nrOps;
}

} // namespace fms::solvers::forward
