#include "solvers/forwardheuristic.h"

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "delayGraph/builder.hpp"
#include "delayGraph/delayGraph.h"
#include "delayGraph/export_utilities.h"
#include "longest_path.h"
#include "maintenanceheuristic.h"
#include "solvers/utils.hpp"

#include <chrono>
#include <fmt/chrono.h>
#include <fmt/compile.h>

using namespace algorithm;
using namespace std;
using namespace FORPFSSPSD;
using namespace DelayGraph;

PartialSolution ForwardHeuristic::solve(FORPFSSPSD::Instance &problemInstance, const commandLineArgs &args){
    // solve the instance
    LOG("Computation of the schedule started");

    // make a copy of the delaygraph
    if (!problemInstance.isGraphInitialized()) {
        problemInstance.updateDelayGraph(Builder::FORPFSSPSD(problemInstance));
    }
    delayGraph dg = problemInstance.getDelayGraph();

    if (args.verbose >= LOGGER_LEVEL::DEBUG) {
        auto name = fmt::format("input_graph_{}.dot", problemInstance.getProblemName());
        DelayGraph::export_utilities::saveAsDot(dg, name);
    }

    auto [result, ASAPST] = SolversUtils::checkSolutionAndOutputIfFails(problemInstance);
    LOG(fmt::format("Number of vertices in the delay graph is {}", dg.get_number_of_vertices()));

    // We only support a single re-entrant machine in the system so choose the first one
    MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();
    if (problemInstance.getMachineOperations(reentrant_machine).size() > 2) {
        throw std::runtime_error("Multiple re-entrancies not implemented yet");
    }

    DelayGraph::Edges initial_sequence =
            createInitialSequence(problemInstance, reentrant_machine);
    PartialSolution solution({{reentrant_machine, initial_sequence}}, ASAPST);

    const auto &ops = problemInstance.getMachineOperations(reentrant_machine);
    const auto &jobs = problemInstance.getJobsOutput();

    // iteratively schedule eligible nodes (insert higher passes between the existing sequence)
    for (std::size_t i = 0; i + 1 < jobs.size(); i++) {
        bool first = true; // First operation is already included in the initial sequence
        for (OperationId op : ops) {
            if(!first) {
                solution = scheduleOneOperation(dg, problemInstance, solution, {jobs[i], op}, args);
            }
            first = false;
        }
    }

    switch (args.algorithm){
        case AlgorithmType::MIBHCS:
        case AlgorithmType::MISIM:
        case AlgorithmType::MIASAP:
        case AlgorithmType::MIASAPSIM:
            std::tie(solution,dg) = MaintenanceHeuristic::triggerMaintenance(dg, problemInstance, reentrant_machine, solution, args);
            problemInstance.updateDelayGraph(dg);
            break;
        default:
            break;
    }

    auto edges = problemInstance.infer_pim_edges(solution);
    auto &solutionMachinesEdges = solution.getChosenEdgesPerMachine();
    auto &solutionEdges = solutionMachinesEdges[problemInstance.getMachines().front()];
    solutionEdges.insert(solutionEdges.end(), edges.begin(), edges.end());

    if(args.verbose >= LOGGER_LEVEL::DEBUG) {
        auto name = fmt::format("output_graph_{}.dot", problemInstance.getProblemName());
        DelayGraph::export_utilities::saveAsDot(problemInstance, solution, name);
    }

    return solution;
}

DelayGraph::Edges
ForwardHeuristic::createInitialSequence(const FORPFSSPSD::Instance &problemInstance,
                                        MachineId reEntrantMachine) {
    DelayGraph::Edges initialSequence;
    const delayGraph & dg = problemInstance.getDelayGraph();
    ReEntrantId reEntrantMachineId = problemInstance.findMachineReEntrantId(reEntrantMachine);

    // check how many operations are mapped
    const std::vector<OperationId> &ops = problemInstance.getMachineOperations(reEntrantMachine);

    /// no ordering is required if the number of operations mapped is 1
    if (ops.size() <= 1) {
        throw std::runtime_error(fmt::format("Machine {} is not re-entrant", reEntrantMachine));
    }

    std::optional<JobId> lastDuplexJob;

    // Add all first passes (of _duplex_ jobs) to the initial sequence
    for (auto job : problemInstance.getJobsOutput()) { // For all but the last job
        if (problemInstance.getPlexity(job, reEntrantMachineId) == FORPFSSPSD::Plexity::DUPLEX) {
            const auto &vTo = dg.get_vertex({job, ops[0]});

            // True on first iteration
            if (!lastDuplexJob.has_value()) {
                const auto &vFrom = dg.get_source(problemInstance.getMachine(ops[0]));
                initialSequence.emplace_back(vFrom.id, vTo.id, 0);
            } else {
                const auto &vFrom = dg.get_vertex({lastDuplexJob.value(), ops[0]});
                initialSequence.push_back(vFrom.get_outgoing_edge(vTo));
            }

            lastDuplexJob = job;
        }
    }

    if (!lastDuplexJob.has_value()) {
        throw FmsSchedulerException("Nothing to schedule; only simplex sheets!");
    }

    // For the last duplex job add its remaining passes to the initial sequence
    for (size_t i = 0; i + 1 < ops.size(); ++i) {
        const auto &vFrom = dg.get_vertex({lastDuplexJob.value(), ops[i]});
        const auto &vTo = dg.get_vertex({lastDuplexJob.value(), ops[i + 1]});
        initialSequence.push_back(vFrom.get_outgoing_edge(vTo));
    }

    return initialSequence;
}

std::pair<DelayGraph::edge, std::vector<option>>
ForwardHeuristic::createOptions(const delayGraph &dg,
                                const FORPFSSPSD::Instance &problem,
                                const PartialSolution &solution,
                                const vertex &eligibleOperation,
                                MachineId reEntrantMachineId) {
    DelayGraph::edge last_potentially_feasible_option =
            solution.getChosenEdges(reEntrantMachineId).back();

    if (Logger::getLevel() <= LOGGER_LEVEL::INFO) {
        // Converting the edges to string is very expensive so we only do it if the log level is 
        // the appropriate level
        LOG(chosenEdgesToString(solution, dg));
    }

    // create a set of potentially feasible options (i.e. replace an edge by two edges),
    // and check whether they are feasible as new partial solutions
    std::vector<option> options;
    delay totalOperationTime = 0;

    // we cannot schedule before the last inserted edge
    const delay currentDeadline = determine_smallest_deadline(eligibleOperation);
    const auto first_possible_edge = solution.first_possible_edge(reEntrantMachineId);
    for (auto e = first_possible_edge; e != solution.getChosenEdges(reEntrantMachineId).cend();
         e++) {
        last_potentially_feasible_option = *e;

        // keep track of the previous and next vertex

        const auto &curV = eligibleOperation;
        const auto &prevV = dg.get_vertex(last_potentially_feasible_option.src);
        const auto &nextV = dg.get_vertex(last_potentially_feasible_option.dst);

        // not allowed to create an option in case of a flush!
        if (curV.operation.jobId != nextV.operation.jobId) {

            // avoid inconsistent total ordering (this should be an invariant in the creation of options...)
            DelayGraph::edge ex_y(prevV.id, curV.id, 0);
            DelayGraph::edge ey_x1(curV.id, nextV.id, 0);
            auto distance = std::distance(solution.getChosenEdges(reEntrantMachineId).begin(), e);
            LOG(fmt::format(FMT_COMPILE("Creating option {}->{}->{}: {}"),
                            prevV.operation,
                            curV.operation,
                            nextV.operation,
                            distance));
            option c(ex_y, ey_x1, prevV.id, curV.id, nextV.id, distance);

            c.prevE.weight = problem.query(prevV, curV);
            c.nextE.weight = problem.query(curV, nextV);

            if (prevV.operation.jobId != nextV.operation.jobId
                && problem.query(prevV, nextV) > c.nextE.weight + c.prevE.weight) {
                LOG(LOGGER_LEVEL::WARNING,
                    fmt::format("Triangle inequality violated! {} -> {} = {} > {} -> {} -> {} = {}",
                                prevV.operation,
                                nextV.operation,
                                problem.query(prevV, nextV),
                                prevV.operation,
                                curV.operation,
                                nextV.operation,
                                c.nextE.weight + c.prevE.weight));
            }
            options.push_back(c);
        }

        if(totalOperationTime > currentDeadline) {
            // As the currentDeadline would have been expired if we would schedule the operation here,
            // we can stop searching for options: any schedule must be infeasible.

            // cannot become feasible anymore; stop adding interleaving options to this partial solution
            break;
        }
        totalOperationTime += (*e).weight;
    }

    return {last_potentially_feasible_option, options};
}

std::optional<std::pair<PartialSolution, option>>
ForwardHeuristic::evaluate_option_feasibility(delayGraph &dg,
                                              const FORPFSSPSD::Instance &problem,
                                              const PartialSolution &solution,
                                              const option &options,
                                              const std::vector<delay> &ASAPTimes,
                                              MachineId reEntrantMachine){
    vector<option> optionsvec{options};
    std::optional<std::pair<PartialSolution, option>> feasibleSolution;
    std::vector<std::pair<PartialSolution, option>> newGenerationOfSolutions = evaluate_option_feasibility(dg,problem,solution,optionsvec,ASAPTimes,reEntrantMachine);
    if (!newGenerationOfSolutions.empty()){
        feasibleSolution = newGenerationOfSolutions[0];
    }
    return feasibleSolution;
    }

std::vector<std::pair<PartialSolution, option>>
ForwardHeuristic::evaluate_option_feasibility(delayGraph &dg,
                                              const FORPFSSPSD::Instance &problem,
                                              const PartialSolution &solution,
                                              const std::vector<option> &options,
                                              const std::vector<delay> &ASAPTimes,
                                              MachineId reEntrantMachine) {
    unsigned int nrFeasibleOptions = 0;
    unsigned int nrInfeasibleOptions = 0;

    std::vector<std::pair<PartialSolution, option>> newGenerationOfSolutions;
    for(const option &o : options)
    {
        std::vector<delay> ASAPST = ASAPTimes; // create a local copy that we can modify without issues

        // Add the edges from the options to the list. ASAPTimes are not (yet) valid for the updated
        // solution... but we will use them only for the chosen edges.
        PartialSolution ps = solution.add(reEntrantMachine, o, ASAPTimes);
        // make a copy of the chosen edges
        DelayGraph::Edges final_sequence = ps.getChosenEdges(reEntrantMachine);

        for(const auto &e : problem.infer_pim_edges(ps)) {
            final_sequence.push_back(e);
        }

        const auto &curV = dg.get_vertex(o.curV);
        const auto &prevV = dg.get_vertex(o.prevV);
        const auto &nextV = dg.get_vertex(o.nextV);

        LOG(LOGGER_LEVEL::DEBUG,
            fmt::format(FMT_COMPILE("Checking feasibility of interleaving {} between {} and "
                                    "{}. The weights of edges are {} and {}"),
                        curV.operation,
                        prevV.operation,
                        nextV.operation,
                        o.prevE.weight,
                        o.nextE.weight));
        const JobId job_start = curV.operation.jobId;

        VerticesCRef origin{dg.get_vertex(FORPFSSPSD::operation{0, 0})};
        VerticesCRef sourcevertices =
                (job_start == 0) ? origin : dg.cget_vertices(std::max(job_start, 1U) - 1);
        VerticesCRef windowvertices =
                dg.cget_vertices(job_start, dg.get_vertex(o.nextE.dst).operation.jobId);

        auto m = dg.get_maint_vertices();
        windowvertices.insert(windowvertices.end(), m.begin(), m.end());

        auto result = ForwardHeuristic::validateInterleaving(
                dg, problem, final_sequence, ASAPST, sourcevertices, windowvertices);

        delay interleaved_starting_time = ASAPST[o.curV];

        if(result.positiveCycle.empty())
        {
            PartialSolution p_sol = solution.add(reEntrantMachine, o, ASAPST);
            p_sol.setMakespanLastScheduledJob(interleaved_starting_time);

            // set the (relaxed) starting time of the interleaved operation and remaining flexibility
            auto [avgProd, nrJobs] =
                    computeFutureAvgProductivy(dg, ASAPST, p_sol, reEntrantMachine);

            p_sol.setAverageProductivity(avgProd/nrJobs);
            p_sol.setNrOpsInLoop(nrJobs);
            p_sol.setEarliestStartFutureOperation(ASAPST[o.nextV]);

            newGenerationOfSolutions.emplace_back(p_sol, o);
            nrFeasibleOptions ++;
        } else {
            LOG(LOGGER_LEVEL::DEBUG,
                fmt::format(
                        FMT_COMPILE("Skipping infeasible option {}->{}->{} with partial makespan "),
                        prevV.operation,
                        curV.operation,
                        nextV.operation,
                        interleaved_starting_time));
            nrInfeasibleOptions ++;
        }
    }
    LOG(LOGGER_LEVEL::DEBUG, fmt::format(FMT_COMPILE("Infeasible: {}"), nrInfeasibleOptions));
    return newGenerationOfSolutions;
}

delay ForwardHeuristic::determine_smallest_deadline(const vertex &v) {
    delay currentDeadline = std::numeric_limits<delay>::max(); // equivalent to the maximal permissible deadline
    // look for the smallest incoming edge with negative value (i.e. outgoing deadline)
    for (const auto &[_, weight] : v.get_outgoing_edges()) {
        if(weight < 0) {
            currentDeadline = std::min(currentDeadline, -weight);
        }
    }
    return currentDeadline;
}

PartialSolution ForwardHeuristic::scheduleOneOperation(delayGraph &dg,
                                                       FORPFSSPSD::Instance &problem,
                                                       const PartialSolution &solution,
                                                       const operation &eligibleOperation,
                                                       const commandLineArgs &args) {
    using a_clock = std::chrono::steady_clock;
    a_clock::time_point start = a_clock::now();

    LOG(LOGGER_LEVEL::INFO,
        fmt::format(FMT_COMPILE("Starting from current solution: {}"), solution));

    auto reEntrantMachineId = problem.getMachine(eligibleOperation);
    auto [solutions, minSolId] =
            getFeasibleOptions(dg, problem, dg.get_vertex(eligibleOperation), solution, args);

    LOG(LOGGER_LEVEL::DEBUG, fmt::format(FMT_COMPILE("*** nr option: {}"), solutions.size()));

    if (!minSolId.has_value()) { // none of the solutions were feasible...
        // create a local copy that we can modify without issues
        auto result = LongestPath::getPositiveCycle(dg, solution.getAllChosenEdges());
        export_utilities::saveAsDot(problem,
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
                    dg.get_vertex((*(bestSolution.first_possible_edge(reEntrantMachineId) - 1)).src)
                            .operation,
                    std::chrono::duration_cast<std::chrono::milliseconds>(end - start)));
    return bestSolution;
}

std::pair<delay, unsigned int>
ForwardHeuristic::computeFutureAvgProductivy(delayGraph &dg,
                                             const std::vector<delay> &ASAPST,
                                             const PartialSolution &ps,
                                             MachineId reEntrantMachineId) {
    const auto &nextV = dg.get_vertex(ps.first_possible_edge(reEntrantMachineId)->dst);
    const auto &curV = dg.get_vertex(ps.first_possible_edge(reEntrantMachineId)->src);
    const auto &prevV = dg.get_vertex((ps.first_possible_edge(reEntrantMachineId) - 1)->src);

    auto iter = ps.first_possible_edge(reEntrantMachineId);

    delay sum = (*iter).weight;

    auto eligibleOp = dg.get_vertex((*iter).src).operation;

    FORPFSSPSD::operation end {eligibleOp.jobId, eligibleOp.operationId - 1}; // the job's predecessor
    iter--;
    int nrOps = 1;
    for (; dg.get_vertex((*iter).src).operation != end && !dg.is_source(dg.get_vertex((*iter).src));
         --iter) {
        sum += (*iter).weight;
        nrOps++;
    }

    delay used_buffer_time = 0;
    FORPFSSPSD::operation op1{nextV.operation.jobId, curV.operation.operationId};
    FORPFSSPSD::operation op2{curV.operation.jobId, curV.operation.operationId - 1};
    if (dg.has_vertex(op1) && dg.has_vertex(op2)) {
        used_buffer_time = ASAPST[dg.get_vertex_id(op1)] - ASAPST[dg.get_vertex_id(op2)];
    }

    return {used_buffer_time, nrOps};
}

LongestPathResult
ForwardHeuristic::validateInterleaving(delayGraph &dg,
                                       const FORPFSSPSD::Instance &problem,
                                       const std::vector<DelayGraph::edge> &InputEdges,
                                       vector<delay> &ASAPST,
                                       const DelayGraph::VerticesCRef &sources,
                                       const DelayGraph::VerticesCRef &window) {
    const auto& maintPolicy = problem.maintenancePolicy();
    DelayGraph::Edges edges;
    // insert the edges to the graph
    for (const auto &i : InputEdges) {
        if(!dg.has_edge(i.src, i.dst)) {
            dg.add_edge(i);
            edges.emplace_back(i);
        }
        if(dg.is_maint(i.src)){
            delay dueWeight = maintPolicy.getMaintDuration((dg.get_vertex(i.src)).operation.maintId)
                              + maintPolicy.getMinimumIdle() - 1;
            edges.emplace_back(dg.add_edge(i.dst, i.src, -dueWeight));
      }
    }

    // Compute the updated ASAP times and check the bounds
    auto result = LongestPath::computeASAPST(dg, ASAPST, sources, window);

    for(const auto& i : edges) {
        dg.remove_edge(i);
    } 

    return result;
}
std::optional<std::size_t> ForwardHeuristic::rankSolutionsASAP(
        std::vector<std::tuple<PartialSolution, option, std::shared_ptr<DelayGraph::delayGraph>>>
                &solutions) {
    // rank all options, normalized
    delay minStart = std::numeric_limits<delay>::max();

    // select the solution with the lowest rank
    std::optional<std::size_t> minRankId;

    for (std::size_t i = 0; i < solutions.size(); ++i) {
        auto &[sol, c, mdg] = solutions[i];
        const auto &ASAPST = sol.getASAPST();
        delay start = std::numeric_limits<delay>::max();

        start = ASAPST[c.curV]; 

        // select the solution with minimum slack:
        if (start <= minStart) {
            minStart = start;
            minRankId = i;
            // dg = mdg; // update delay graph to chosen
        }
    }

    return minRankId;
}

std::optional<std::size_t> algorithm::ForwardHeuristic::rankSolutions(
        std::vector<std::tuple<PartialSolution, option, std::shared_ptr<DelayGraph::delayGraph>>>
                &solutions,
        PathTimes &ASAPTimes,
        FORPFSSPSD::MachineId reEntrantMachine,
        const commandLineArgs &args) {
    // rank all options, normalized
    delay minPush = std::numeric_limits<delay>::max();
    delay maxPush = std::numeric_limits<delay>::min();
    delay minPushNext = std::numeric_limits<delay>::max();
    delay maxPushNext = std::numeric_limits<delay>::min();

    auto minOpsInBuffer = std::numeric_limits<uint32_t>::max();
    auto maxOpsInBuffer = std::numeric_limits<uint32_t>::min();

    for (const auto &[sol, c, mdg] : solutions) {
        const auto &curV = mdg->get_vertex(c.curV);
        const auto &ASAPST = sol.getASAPST();
        auto eligibleOp = curV.operation;
        delay push = ASAPST[c.curV] - ASAPTimes[c.curV];
        delay push_next = ASAPST[c.nextV] - ASAPTimes[c.nextV];

        auto iter = sol.first_possible_edge(reEntrantMachine);

        // the job's predecessor
        FORPFSSPSD::operation end{eligibleOp.jobId, eligibleOp.operationId - 1};
        iter--;
        uint32_t nrOps = 1;
        for (; !mdg->is_source(iter->src) && mdg->get_vertex(iter->src).operation != end; --iter) {
            nrOps++;
        }

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
        const auto &ASAPST = sol.getASAPST();
        delay push = ASAPST[c.curV] - ASAPTimes[c.curV];
        delay push_next = ASAPST[c.nextV] - ASAPTimes[c.nextV];

        auto eligibleOp = mdg->get_vertex(c.curV).operation;

        auto iter = sol.first_possible_edge(reEntrantMachine);

        FORPFSSPSD::operation end{eligibleOp.jobId,
                                  eligibleOp.operationId - 1}; // the job's predecessor
        iter--;
        uint32_t nrOps = 1;
        for (; !mdg->is_source(iter->src) && mdg->get_vertex(iter->src).operation != end; --iter) {
            nrOps++;
        }

        delay interleavedStartingTime = ASAPST[c.curV];
        sol.setMakespanLastScheduledJob(interleavedStartingTime);
        sol.setEarliestStartFutureOperation(push);
        sol.setNrOpsInLoop(nrOps);

        LOG(LOGGER_LEVEL::INFO,
            fmt::format(FMT_COMPILE("Earliest current op time: {}, earliest future op time: {}, "
                                    "push_next: {}, nr ops committed "),
                        ASAPST[c.curV],
                        ASAPST[c.nextV],
                        push_next,
                        nrOps));

        auto pushRange = static_cast<double>((maxPush != minPush) ? (maxPush - minPush) : 1);
        auto pushNextRange =
                static_cast<double>((maxPushNext != minPushNext) ? (maxPushNext - minPushNext) : 1);
        auto nrOpsRange = static_cast<double>(
                (maxOpsInBuffer != minOpsInBuffer) ? (maxOpsInBuffer - minOpsInBuffer) : 1);

        auto pushNorm = static_cast<double>(push - minPush) / pushRange;
        auto pushNextNorm = static_cast<double>(push_next - minPushNext) / pushNextRange;
        auto nrOpsNorm = static_cast<double>(nrOps - minOpsInBuffer) / nrOpsRange;

        LOG(LOGGER_LEVEL::INFO,
            fmt::format(FMT_COMPILE("Push (norm.): {}, push_next (norm.): {}, nrOps (norm): {}"),
                        pushNorm,
                        pushNextNorm,
                        nrOpsNorm));

        double curRank =
                // minimize effort to come to this scheduling decision
                args.flexibilityWeight * pushNorm
                // minimize effort of executing committed work
                + args.productivityWeight * pushNextNorm
                // maximize size of committed work
                + args.tieWeight * nrOpsNorm;

        sol.setRanking(curRank);

        const auto &prevV = mdg->get_vertex(c.prevV);
        const auto &nextV = mdg->get_vertex(c.nextV);
        LOG(LOGGER_LEVEL::INFO,
            fmt::format(FMT_COMPILE("Rank: (norm.): {} - {}, {}"),
                        curRank,
                        prevV.operation,
                        nextV.operation));

        // select the solution with lowest rank:
        if (curRank < minRank) {
            minRank = curRank;
            minRankId = i;
            // dg = mdg; // update delay graph to chosen
        }
    }

    return minRankId;
}

std::tuple<std::vector<std::tuple<PartialSolution, std::shared_ptr<DelayGraph::delayGraph>>>,
           std::optional<std::size_t>>
algorithm::ForwardHeuristic::getFeasibleOptions(delayGraph &dg,
                                                const FORPFSSPSD::Instance &problem,
                                                const vertex &eligibleOperation,
                                                const PartialSolution &solution,
                                                const commandLineArgs &args) {
    MachineId reEntrantMachineId = problem.getMachine(eligibleOperation.operation);

    // create all options that are potentially feasible:
    auto [lastPotentiallyFeasibleOption, options] =
            createOptions(dg, problem, solution, eligibleOperation, reEntrantMachineId);

    // update the ASAPTimes for the coming window, so that we have enough information to compute the
    // ranking
    const JobId job_start = eligibleOperation.operation.jobId;
    vector<delay> ASAPTimes = solution.getASAPST();

    LongestPath::computeASAPST(
            dg,
            ASAPTimes,
            dg.cget_vertices(max(job_start, 1U) - 1),
            dg.cget_vertices(job_start,
                            dg.get_vertex(lastPotentiallyFeasibleOption.dst).operation.jobId));

    std::vector<std::pair<PartialSolution, option>> generationOfSolutions =
            evaluate_option_feasibility(
                    dg, problem, solution, options, ASAPTimes, reEntrantMachineId);
    std::vector<std::tuple<PartialSolution, option, std::shared_ptr<delayGraph>>>
            newGenerationOfSolutions;

    auto dgPtr = std::make_shared<delayGraph>(std::move(dg));
    for (auto &[sol, opt] : generationOfSolutions) {
        switch (args.algorithm) {
            case AlgorithmType::MIBHCS:
            case AlgorithmType::MIASAP:{
                auto [maintSolution, maintDg] =
                        MaintenanceHeuristic::triggerMaintenance(*dgPtr, problem, sol, opt, args);
                newGenerationOfSolutions.emplace_back(
                        maintSolution, opt, std::make_shared<delayGraph>(std::move(maintDg)));
                break;
            }
            default: {
                newGenerationOfSolutions.emplace_back(sol, opt, dgPtr);
            }
        }
    }

    std::optional<std::size_t> minRankId;
    switch (args.algorithm){
        case AlgorithmType::ASAP:
        case AlgorithmType::MIASAP:
        case AlgorithmType::MIASAPSIM:
            minRankId = rankSolutionsASAP(newGenerationOfSolutions);
            break;
        default:
            minRankId = rankSolutions(newGenerationOfSolutions, ASAPTimes, reEntrantMachineId, args);
            break;
    }
    

    std::vector<std::tuple<PartialSolution, std::shared_ptr<DelayGraph::delayGraph>>> result;
    result.reserve(newGenerationOfSolutions.size());
    for (auto &[sol, _, dg] : newGenerationOfSolutions) {
        if ((args.algorithm != AlgorithmType::MIBHCS) && (args.algorithm != AlgorithmType::MIASAP)) {
            dg.reset();
        }
        result.emplace_back(std::move(sol), dg);
    }

    dg = std::move(*dgPtr);
    dgPtr.reset();
    return {std::move(result), minRankId};
}
