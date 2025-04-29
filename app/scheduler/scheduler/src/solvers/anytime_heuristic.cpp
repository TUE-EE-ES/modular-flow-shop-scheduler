#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/solvers/anytime_heuristic.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/edge.hpp"
#include "fms/cg/export_utilities.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/problem/operation.hpp"
#include "fms/solvers/forward_heuristic.hpp"
#include "fms/solvers/utils.hpp"

#include <chrono>

namespace fms::solvers::anytime {

PartialSolution solve(problem::Instance &problemInstance, const cli::CLIArgs &args) {
    // solve the instance
    LOG("Computation of the schedule started");

    // make a copy of the delaygraph
    cg::ConstraintGraph dg = problemInstance.getDelayGraph();

    if (IS_LOG_D()) {
        auto name = fmt::format("input_graph_{}.dot", problemInstance.getProblemName());
        cg::exports::saveAsDot(dg, name);
    }

    auto [result, ASAPST] = SolversUtils::checkSolutionAndOutputIfFails(problemInstance);
    LOG(fmt::format("Number of vertices in the delay graph is {}", dg.getNumberOfVertices()));

    // We only support a single re-entrant machine in the system so choose the first one
    problem::MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();
    if (problemInstance.getMachineOperations(reentrant_machine).size() > 2) {
        throw std::runtime_error("Multiple re-entrancies not implemented yet");
    }

    auto initial_sequence = forward::createInitialSequence(problemInstance, reentrant_machine);
    PartialSolution solution({{reentrant_machine, initial_sequence}}, ASAPST);

    const auto &ops = problemInstance.getMachineOperations(reentrant_machine);
    const auto &jobs = problemInstance.getJobsOutput();

    // iteratively schedule eligible nodes (insert higher passes between the existing sequence)
    for (std::size_t i = 0; i + 1 < jobs.size(); i++) {
        bool first = true; // First operation is already included in the initial sequence

        for (problem::OperationId op : ops) {
            if (!first) {
                const auto &eligibleOperation = dg.getVertex({jobs[i], op});
                solution = scheduleOneOperation(
                        dg, problemInstance, solution, eligibleOperation, args);
            }
            first = false;
        }
    }

    // for (auto edge: solution.getChosenEdges(reentrant_machine)){
    //     std::cout << dg.getVertex(edge->src)->as_string() << " " <<
    //     dg.getVertex(edge->dst)->as_string() << " "  << "\n";
    // }

    if (IS_LOG_D()) {
        auto name = fmt::format("output_graph_{}.dot", problemInstance.getProblemName());
        cg::exports::saveAsDot(problemInstance, solution, name);
    }

    return solution;
}

PartialSolution scheduleOneOperation(cg::ConstraintGraph &dg,
                                     problem::Instance &problem,
                                     const PartialSolution &solution,
                                     const cg::Vertex &eligibleOperation,
                                     const cli::CLIArgs &args) {
    // To DO: Rework to fit new scheme
    using a_clock = std::chrono::steady_clock;
    a_clock::time_point start = a_clock::now();

    LOG_I(FMT_COMPILE("Starting from current solution: {}"), solution);

    auto reEntrantMachineId = problem.getMachine(eligibleOperation.operation);
    PartialSolution bestSolution = solution;

    bestSolution = getSolution(dg, problem, eligibleOperation, solution, args);

    a_clock::time_point end = a_clock::now();

    LOG(fmt::format(FMT_COMPILE("Scheduled operation {} after operation {} in {} ms."),
                    eligibleOperation.operation,
                    *(bestSolution.firstPossibleOp(reEntrantMachineId) - 1),
                    std::chrono::duration_cast<std::chrono::milliseconds>(end - start)));
    return bestSolution;
}

PartialSolution getSolution(cg::ConstraintGraph &dg,
                            const problem::Instance &problem,
                            const cg::Vertex &eligibleOperation,
                            const PartialSolution &solution,
                            const cli::CLIArgs &args) {
    problem::MachineId reEntrantMachineId = problem.getMachine(eligibleOperation.operation);

    using a_clock = std::chrono::steady_clock;
    a_clock::time_point start = a_clock::now();

    // create all options that are potentially feasible:
    auto [lastPotentiallyFeasibleOption, options] =
            forward::createOptions(problem, solution, eligibleOperation, reEntrantMachineId);

    auto existingNorms = std::make_tuple(std::numeric_limits<delay>::max(),
                                         std::numeric_limits<delay>::min(),
                                         std::numeric_limits<delay>::max(),
                                         std::numeric_limits<delay>::min(),
                                         std::numeric_limits<uint32_t>::max(),
                                         std::numeric_limits<uint32_t>::min());
    auto existingRank = std::make_tuple(solution,
                                        std::numeric_limits<delay>::max(),
                                        std::numeric_limits<delay>::max(),
                                        std::numeric_limits<uint32_t>::max());

    // fetch first base solution
    SchedulingOption &o = options[0];
    std::optional<std::pair<PartialSolution, SchedulingOption>> newSolution =
            evaluateOption(dg,
                           problem,
                           eligibleOperation,
                           solution,
                           o,
                           lastPotentiallyFeasibleOption,
                           existingNorms,
                           existingRank,
                           args);
    if (!newSolution.has_value()) {
        // the first option should always be feasible for this anytime approach
        throw FmsSchedulerException(fmt::format(
                "First option tried is infeasible in anytime approach. This should not be"
                "possible in the Canon case",
                eligibleOperation.operation));
    }

    // TO DO: should implement options for ways to getNextOption and then

    size_t index = 0;
    std::optional<std::pair<SchedulingOption, size_t>> nextOption = getNextOption(options, index);
    a_clock::time_point end = a_clock::now();
    // while we still have options and we have not timed out
    while (nextOption.has_value()
           && std::chrono::duration_cast<std::chrono::microseconds>(end - start) < args.timeOut) {
        evaluateOption(dg,
                       problem,
                       eligibleOperation,
                       solution,
                       (nextOption.value()).first,
                       lastPotentiallyFeasibleOption,
                       existingNorms,
                       existingRank,
                       args);
        index = (nextOption.value()).second;
        nextOption = getNextOption(options, index);
        end = a_clock::now();
    }

    return std::get<0>(existingRank);
}

std::optional<std::pair<PartialSolution, SchedulingOption>>
evaluateOption(cg::ConstraintGraph &dg,
               const problem::Instance &problem,
               const cg::Vertex &eligibleOperation,
               const PartialSolution &solution,
               const SchedulingOption &o,
               const problem::Operation &lastPotentiallyFeasibleOption,
               std::tuple<delay, delay, delay, delay, uint32_t, uint32_t> &existingNorms,
               std::tuple<PartialSolution, delay, delay, uint32_t> &existingRank,
               const cli::CLIArgs &args) {

    problem::MachineId reEntrantMachineId = problem.getMachine(eligibleOperation.operation);
    // update the ASAPTimes for the coming window, so that we have enough information to compute the
    // ranking

    const problem::JobId jobStart = eligibleOperation.operation.jobId;
    std::vector<delay> ASAPTimes = solution.getASAPST();

    algorithms::paths::computeASAPST(
            dg,
            ASAPTimes,
            dg.getVerticesC(std::max(jobStart, problem::JobId(1U)) - 1),
            dg.getVerticesC(jobStart, lastPotentiallyFeasibleOption.jobId));

    std::optional<std::pair<PartialSolution, SchedulingOption>> newSolution =
            forward::evaluateOptionFeasibility(
                    dg, problem, solution, o, ASAPTimes, reEntrantMachineId);

    if (newSolution.has_value()) {
        std::tie(existingNorms, existingRank) = rankSolution(newSolution.value(),
                                                             dg,
                                                             existingNorms,
                                                             existingRank,
                                                             ASAPTimes,
                                                             reEntrantMachineId,
                                                             args);
    }
    return newSolution;
}

std::optional<std::pair<SchedulingOption, size_t>>
getNextOption(std::vector<SchedulingOption> options, std::size_t index) {

    std::optional<std::pair<SchedulingOption, size_t>> nextOption;
    index++;
    if (index < options.size()) {
        nextOption = std::make_pair(options[index], index);
    }
    return nextOption;
}

cg::Edges getExitEdges() {
    // should decide on what the exit path edges are based on our exit policy
    // the edges here are added to the graph when we validate interleaving
    // edges should be such that we are immediately followed by the next operation we are to
    // schedule in the ordering
    return {};
}

std::tuple<std::tuple<delay, delay, delay, delay, uint32_t, uint32_t>,
           std::tuple<PartialSolution, delay, delay, uint32_t>>
rankSolution(std::tuple<PartialSolution, SchedulingOption> currentSolution,
             cg::ConstraintGraph &dg,
             std::tuple<delay, delay, delay, delay, uint32_t, uint32_t> existingNorms,
             std::tuple<PartialSolution, delay, delay, uint32_t> existingRank,
             algorithms::paths::PathTimes &ASAPTimes,
             problem::MachineId reEntrantMachine,
             const cli::CLIArgs &args) {

    auto [sol, c] = currentSolution;
    auto [minPush, maxPush, minPushNext, maxPushNext, minOpsInBuffer, maxOpsInBuffer] =
            existingNorms;
    auto [curSol, curPush, curPushNext, curNrOps] = existingRank;
    const auto &ASAPST = sol.getASAPST();
    const auto &eligibleOp = c.curO;
    auto curVId = dg.getVertexId(c.curO);
    delay push = ASAPST[curVId] - ASAPTimes[curVId];
    delay push_next = ASAPST[curVId] - ASAPTimes[curVId];

    auto iter = std::make_reverse_iterator(sol.firstPossibleOp(reEntrantMachine));

    problem::Operation end{eligibleOp.jobId, eligibleOp.operationId - 1}; // the job's predecessor
    const auto rEnd = sol.getMachineSequence(reEntrantMachine).rend();
    uint32_t nrOps = 1;
    for (; *iter != end && iter != rEnd; ++iter) {
        nrOps++;
    }

    minPush = std::min(minPush, push);
    maxPush = std::max(maxPush, push);

    minPushNext = std::min(minPushNext, push_next);
    maxPushNext = std::max(maxPushNext, push_next);

    minOpsInBuffer = std::min(minOpsInBuffer, nrOps);
    maxOpsInBuffer = std::max(maxOpsInBuffer, nrOps);

    // select the solution with the lowest rank
    double minRank = std::numeric_limits<double>::max();
    delay interleavedStartingTime = ASAPST[curVId];
    sol.setMakespanLastScheduledJob(interleavedStartingTime);
    sol.setEarliestStartFutureOperation(push);
    sol.setNrOpsInLoop(nrOps);

    LOG_I(FMT_COMPILE("Earliest current op time: {}, earliest future op time: {}, "
                      "push_next: {}, nr ops committed "),
          ASAPST[curVId],
          ASAPST[dg.getVertexId(c.nextO)],
          push_next,
          nrOps);

    auto pushRange = static_cast<double>((maxPush != minPush) ? (maxPush - minPush) : 1);
    auto pushNextRange =
            static_cast<double>((maxPushNext != minPushNext) ? (maxPushNext - minPushNext) : 1);
    auto nrOpsRange = static_cast<double>(
            (maxOpsInBuffer != minOpsInBuffer) ? (maxOpsInBuffer - minOpsInBuffer) : 1);

    auto pushNorm = static_cast<double>(push - minPush) / pushRange;
    auto pushNextNorm = static_cast<double>(push_next - minPushNext) / pushNextRange;
    auto nrOpsNorm = static_cast<double>(nrOps - minOpsInBuffer) / nrOpsRange;

    auto curPushNorm = static_cast<double>(curPush - minPush) / pushRange;
    auto curPushNextNorm = static_cast<double>(curPushNext - minPushNext) / pushNextRange;
    auto curNrOpsNorm = static_cast<double>(curNrOps - minOpsInBuffer) / nrOpsRange;

    LOG_I(FMT_COMPILE("Push (norm.): {}, push_next (norm.): {}, nrOps (norm): {}"),
          pushNorm,
          pushNextNorm,
          nrOpsNorm);

    double curRank =
            // minimize effort to come to this scheduling decision
            args.flexibilityWeight * curPushNorm
            // minimize effort of executing committed work
            + args.productivityWeight * curPushNextNorm
            // maximize size of committed work
            + args.tieWeight * curNrOpsNorm;

    double rank =
            // minimize effort to come to this scheduling decision
            args.flexibilityWeight * pushNorm
            // minimize effort of executing committed work
            + args.productivityWeight * pushNextNorm
            // maximize size of committed work
            + args.tieWeight * nrOpsNorm;

    sol.setRanking(rank);
    LOG_I(FMT_COMPILE("Rank: (norm.): {} - {}, {}"), curRank, c.prevO, c.nextO);

    // select the solution with lowest rank:
    existingNorms = {minPush, maxPush, minPushNext, maxPushNext, minOpsInBuffer, maxOpsInBuffer};
    if (rank < curRank) {
        curRank = rank;
        curSol = sol;
        existingRank = {curSol, push, push_next, nrOps};
    }
    return std::make_tuple(existingNorms, existingRank);
}

} // namespace fms::solvers::anytime
