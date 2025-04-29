#include "fms/pch/containers.hpp"
#include "fms/pch/utils.hpp"

#include "fms/solvers/branch_bound.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/builder.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/export_utilities.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/problem/operation.hpp"
#include "fms/solvers/forward_heuristic.hpp"
#include "fms/solvers/pareto_heuristic.hpp"

#include <fstream>
#include <iomanip> // std::setprecision
#include <iostream>
#include <queue>

namespace fms::solvers::branch_bound {

BranchBoundNode::BranchBoundNode(const problem::Instance &problem,
                                 cg::ConstraintGraph &dg,
                                 const PartialSolution &solution,
                                 delay trivialLowerBound) :
    solution(solution) {
    this->solution.clearASAPST();
    algorithms::paths::PathTimes ASAPST = getASAPST(problem, dg);
    auto reEntrantMachine = problem.getReEntrantMachines().front();

    lastInsertedOperation = *solution.firstPossibleOp(reEntrantMachine);

    // calculate lowerbounds

    // OPTION 1:
    lowerbound = ASAPST.back(); // use the current BFM as the lower bound
    makespan = ASAPST.back();

    // OPTION 2: lower bound of scheduled part + sum of processing times for first and second
    // passes the remaining first pass operations are not scheduled yet there are second pass
    // operations that are not scheduled yet

    // add the minimal time for first passes and the minimal time for second passes
    // the minimal time for first passes needs to be determined for each type of plexity

    // TODO: a lowerbound that can tighten dynamically when scheduling decisions are taken
    // i.e., a combination of the trivial completion time and the current scheduling result
    lowerbound = std::max(lowerbound, trivialLowerBound);
}

std::vector<delay> BranchBoundNode::getASAPST(const problem::Instance &problem,
                                              cg::ConstraintGraph &dg) const {
    std::vector<delay> ASAPST = algorithms::paths::initializeASAPST(dg);
    // determine the sequencing edges
    cg::Edges finalSequence = solution.getAllAndInferredEdges(problem);

    // compute (over the whole window) the ASAPST for these sequencing edges
    // assume it is feasible, otherwise this will take a very long time
    algorithms::paths::LongestPathResult result = forward::validateInterleaving(
            dg, problem, finalSequence, ASAPST, {}, dg.getVerticesC());
    if (!result.positiveCycle.empty()) {
        std::cout << "Detected infeasible edges: " << std::endl;
        for (const auto &edge : result.positiveCycle) {
            LOG(fmt::format("-- {}\n", edge));
        }

        cg::exports::saveAsDot(dg, "inconsistent.dot", finalSequence);
        std::cout << chosenSequencesToString(solution);

        throw FmsSchedulerException(
                "Positive cycle encountered or invalid constraints encountered while "
                "determining lowerbound of partial solution");
    }

    return ASAPST;
}

bool operator>(const BranchBoundNode &lhs, const BranchBoundNode &rhs) {
    return lhs.getLowerbound() > rhs.getLowerbound();
}

PartialSolution solve(problem::Instance &problemInstance, const cli::CLIArgs &args) {
    // solve the instance
    LOG("Started branch and bound");

    // make a copy of the delaygraph
    if (!problemInstance.isGraphInitialized()) {
        problemInstance.updateDelayGraph(cg::Builder::FORPFSSPSD(problemInstance));
    }
    auto dg = problemInstance.getDelayGraph();

    if (args.verbose >= utils::LOGGER_LEVEL::DEBUG) {
        cg::exports::saveAsTikz(problemInstance, dg, "input_graph.tex");
    }

    std::vector<delay> ASAPST = algorithms::paths::initializeASAPST(dg);
    auto result = algorithms::paths::computeASAPST(dg, ASAPST);

    // check wether the input graph is feasible or not
    if (!result.positiveCycle.empty()) {
        LOG_C("The input graph is infeasible. Aborting.");
        throw FmsSchedulerException("The input graph is infeasible. Aborting.");
    }

    LOG(std::string() + "Number of vertices in the delay graph is "
        + std::to_string(dg.getNumberOfVertices()));

    // find out which machine is the re-entrant machine
    problem::MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();
    const std::vector<unsigned int> &ops = problemInstance.getOperationsMappedOnMachine().at(
            reentrant_machine); // check how many operations are mapped

    auto initial_sequence = forward::createInitialSequence(problemInstance, reentrant_machine);

    auto trivialLowerBound = createTrivialCompletionLowerBound(problemInstance);

    // best-first can be implemented by a priority_queue
    //    std::priority_queue<BranchBoundNode, std::vector<BranchBoundNode>,
    //    std::greater<BranchBoundNode>> open_nodes; // use a min-heap to retrieve the solution with
    //    the smallest lower bound
    // LIFO (depth-first)
    std::vector<BranchBoundNode>
            open_nodes; // use a 'stack' to continue with the last added node first
    open_nodes.emplace_back(problemInstance,
                            dg,
                            PartialSolution({{reentrant_machine, initial_sequence}}, ASAPST),
                            trivialLowerBound);

    LOG_I("Using INITIAL SCHEDULING to get initial result");

    // The Branch & Bound algorithm can be seeded by creating any initial schedule (e.g., the
    // 'stupid schedule', a bhcs/md-bhcs result)
    BranchBoundNode stupidScheduleNode(
            createStupidSchedule(problemInstance, reentrant_machine, trivialLowerBound));
    BranchBoundNode bhcsNode(
            problemInstance, dg, forward::solve(problemInstance, args), trivialLowerBound);
    LOG_C("Seed with BHCS completed with makespan of {}", bhcsNode.getMakespan());

    // The Branch & Bound algorithm is seeded with the best result from the Pareto scheduler
    cli::CLIArgs argss = args;
    argss.maxPartialSolutions = 20;
    std::vector<PartialSolution> solutions = ParetoHeuristic::solve(problemInstance, argss);
    PartialSolution best = solutions.back();
    for (const PartialSolution &sol : solutions) {
        if (best.getMakespan() > sol.getMakespan()) {
            best = sol;
        }
    }
    LOG_C("Seed with MD-BHCS completed with makespan of {}", best.getMakespan());

    BranchBoundNode mdbhcsNode(problemInstance, dg, best, trivialLowerBound);

    BranchBoundNode bestFoundNode = mdbhcsNode; // Choose which algorithm's result to seed with
    if (bestFoundNode.getMakespan() > bhcsNode.getMakespan()) {
        bestFoundNode = bhcsNode;
    }
    if (bestFoundNode.getMakespan() < open_nodes.back().getLowerbound()) {
        LOG_C("{} is smaller than initial lowerbound {}",
              bestFoundNode.getMakespan(),
              open_nodes.back().getLowerbound());
        throw FmsSchedulerException(
                "Either the initial lowerbound or the initial solution is incorrect; found a "
                "(valid?) solution that is lower than the initial lower bound");
    }
    LOG_C("Finished INITIAL SCHEDULING heuristic with makespan {}", bestFoundNode.getMakespan());

    auto start = utils::time::getCpuTime();

    delay previous_iteration_lowerbound = 0;

    unsigned int iteration = 0;
    unsigned int retired = 0;
    while (!open_nodes.empty()) {
        delay lowerbound = bestFoundNode.getMakespan();
        for (const BranchBoundNode &n : open_nodes) {
            lowerbound = std::min(lowerbound, n.getLowerbound());
        }

        LOG_I("Open nodes: {}", open_nodes.size());
        const BranchBoundNode node = open_nodes.back();
        open_nodes.pop_back();

        PartialSolution solution = node.getSolution();
        // recompute the solution ASAPST every time to save a lot of memory (feasible sequences are
        // cheap to compute anyway)
        solution.setASAPST(node.getASAPST(problemInstance, dg));

        // sanity check on the lowerbound values calculated. Exploring the solution space further
        // must only lead to stricter lowerbounds, never looser ones
        if (previous_iteration_lowerbound > lowerbound) {
            throw FmsSchedulerException(
                    std::string()
                    + "Lower bound decreased! This cannot happen with a proper lower bound!"
                    + std::to_string(previous_iteration_lowerbound) + " > "
                    + std::to_string(lowerbound));
        } else {
            if (previous_iteration_lowerbound
                != lowerbound) { // write to a file, the lowerbound has been changed
                std::ofstream stream(args.outputFile + ".lb");
                stream << std::min(lowerbound, bestFoundNode.getMakespan());
                stream.close();
            }
            previous_iteration_lowerbound = lowerbound;
        }

        if (lowerbound >= bestFoundNode.getMakespan()) {
            // found an optimal solution, finish execution!
            LOG_C("Optimal solution found");
            return {bestFoundNode.getSolution().getChosenSequencesPerMachine(),
                    bestFoundNode.getASAPST(problemInstance, dg)};
        }
        if ((iteration % 800) == 0) {
            std::cout << std::setw(12) << "ITERATION" << std::setw(15) << "LOWERBOUND"
                      << std::setw(15) << "BEST FOUND" << std::setw(12) << "GAP (%)"
                      << std::setw(12) << "NODES LEFT" << std::setw(16) << "NODES RETIRED"
                      << std::setw(18) << "TIME SPENT (s)" << std::setw(22) << "TIME SPENT/NODE (s)"
                      << std::endl;
        }

        if (((iteration++) + 1) % 40 == 0) {
            auto time_spent = utils::time::getCpuTime() - start;
            auto precision = std::cout.precision(4);

            std::cout << std::setw(12) << iteration << std::setw(15) << lowerbound << std::setw(15)
                      << bestFoundNode.getMakespan() << std::setw(12)
                      << (((double)bestFoundNode.getMakespan() - lowerbound) / (double)lowerbound)
                                 * 100
                      << std::setw(12) << open_nodes.size() << std::setw(16) << retired
                      << std::setw(18) << time_spent << std::setw(22) << time_spent / iteration
                      << std::endl;

            std::cout << std::setprecision(precision);
            if (time_spent > args.timeOut) {
                LOG_C("Time limit exceeded");
                return {bestFoundNode.getSolution().getChosenSequencesPerMachine(),
                        bestFoundNode.getASAPST(problemInstance, dg)};
            }
        }

        if (bestFoundNode.getASAPST(problemInstance, dg).at(dg.getVertices().back().id)
            <= node.getLowerbound()) {
            // ignore this branch, as it can never become optimal
            retired++;
            continue;
        }

        bool scheduled_one_operation = false;
        // schedule one eligible node
        for (std::size_t i = 0; i < problemInstance.getNumberOfJobs() - 1; i++) {
            if (scheduled_one_operation) {
                break;
            }

            const auto jobId = problemInstance.getJobAtOutputPosition(i);

            bool first = true; // First operation is already included in the initial sequence
            for (unsigned int op : ops) {
                if (!first) {
                    problem::JobId firstPossibleJob =
                            solution.firstPossibleOp(reentrant_machine)->jobId;
                    if (dg.isSource(*solution.firstPossibleOp(reentrant_machine))
                        || jobId > firstPossibleJob) {
                        // schedule the next operation
                        const auto &eligibleOperation = dg.getVertex({jobId, op});
                        auto newSolutions = scheduleOneOperation(
                                dg, problemInstance, solution, eligibleOperation);

                        if (i + 2
                            == problemInstance
                                       .getNumberOfJobs()) { // if it was the (second-to-)last sheet
                                                             // to schedule (last second pass is
                                                             // already included)
                            for (const auto &s : newSolutions) {
                                BranchBoundNode new_node(problemInstance, dg, s, trivialLowerBound);
                                if (bestFoundNode.getMakespan() > new_node.getMakespan()) {
                                    LOG_W("Found a better solution {} is smaller than {} ",
                                          new_node.getMakespan(),
                                          bestFoundNode.getMakespan());

                                    bestFoundNode = new_node;
                                }
                            }
                        } else {
                            // add each of the feasible solutions to the heap of open nodes
                            LOG_I("Adding {} nodes", newSolutions.size());

                            // for the stack-based branch and bound the order of insertion makes a
                            // big difference, the one that is more likely to be optimal should be
                            // evaluated earlier
                            for (PartialSolution &s : newSolutions) {
                                BranchBoundNode newNode(problemInstance, dg, s, trivialLowerBound);
                                std::string edges1 = chosenSequencesToString(solution);
                                std::string edges2 = chosenSequencesToString(s);

                                if (newNode.getLowerbound() < node.getLowerbound()) {
                                    LOG_C("Lower bound decreased by inserting an operation!");
                                    LOG_C("{} -> {}",
                                          *(s.firstPossibleOp(reentrant_machine) - 1),
                                          *s.firstPossibleOp(reentrant_machine));
                                    LOG_I("original node: {}: {}", edges1, node.getLowerbound());
                                    LOG_I("new node: {}: {}", edges2, newNode.getLowerbound());

                                    std::ofstream before("before_insertion.txt");
                                    before << edges1;
                                    before.close();
                                    std::ofstream after("after_insertion.txt");
                                    after << edges2;
                                    after.close();

                                    throw FmsSchedulerException(
                                            "Lower bound decreased by making a scheduling "
                                            "decision! This cannot happen with a proper lower "
                                            "bound!");
                                }

                                if (newNode.getLowerbound() < bestFoundNode.getMakespan()) {
                                    open_nodes.push_back(newNode);
                                } else {
                                    retired++;
                                }
                            }
                        }
                        scheduled_one_operation = true;
                        break; // Break out of the loop after scheduling the current operation,
                               // continue with another Branch and Bound node
                    }
                }
                first = false;
            }
        }
    }

    if (IS_LOG_D()) {
        cg::exports::saveAsTikz(problemInstance, dg, "output_graph.tex");
    }

    std::ofstream stream(args.outputFile + ".lb");
    stream << bestFoundNode.getMakespan();
    stream.close();

    LOG_C("Optimal solution found (no more branches left to explore)");
    return {bestFoundNode.getSolution().getChosenSequencesPerMachine(),
            bestFoundNode.getASAPST(problemInstance, dg)};
}

delay createTrivialCompletionLowerBound(const problem::Instance &problem) {
    delay first_pass_processing_time = 0;
    delay second_pass_processing_time = 0;
    std::optional<problem::JobId> firstDuplex;
    const auto &dg = problem.getDelayGraph();

    for (unsigned int i = 0; i < problem.getNumberOfJobs(); i++) {
        const problem::JobId jobId(i);
        // FIXME: this is created specifically for CPP cases:

        if (!firstDuplex && problem.getReEntrancies(jobId) == problem::plexity::DUPLEX) {
            firstDuplex = jobId;
        }

        if (firstDuplex) {
            if (dg.hasVertex({jobId, 1})) {
                first_pass_processing_time += problem.getProcessingTime({jobId, 1});
            }
            second_pass_processing_time += problem.getProcessingTime({jobId, 2});
            // TODO: also add half of the minimum incoming and outgoing setup times
        }
    }

    auto result = algorithms::paths::computeASAPST(dg);

    delay firstDuplexStartTime = result.times.at(dg.getSource(problem::MachineId(1)).id);
    if (firstDuplex) {
        firstDuplexStartTime = result.times[dg.getVertex({*firstDuplex, 1}).id];
    }

    const auto &jobsOutput = problem.getJobsOutput();
    delay lastSheetUnloadTime =
            problem.getSetupTime({jobsOutput.back(), 2}, {jobsOutput.back(), 3});

    delay lowerbound = firstDuplexStartTime + first_pass_processing_time
                       + second_pass_processing_time + lastSheetUnloadTime;

    return std::max(lowerbound, result.times.back());
}

std::vector<PartialSolution>
ranked(cg::ConstraintGraph &dg,
       const problem::Instance &problemInstance,
       const std::vector<std::pair<PartialSolution, SchedulingOption>> &generationOfSolutions,
       const std::vector<delay> &ASAPTimes) {
    using ranked_solution = std::pair<PartialSolution, double>;
    auto comp = [](const ranked_solution &a, const ranked_solution &b) {
        return a.second < b.second;
    }; // rank highest to lowest
    std::priority_queue<ranked_solution, std::vector<ranked_solution>, decltype(comp)> ranking_heap(
            comp);

    auto reEntrantMachine = problemInstance.getReEntrantMachines().front();

    // rank all options, normalized
    delay min_push = std::numeric_limits<delay>::max();
    delay max_push = std::numeric_limits<delay>::min();
    delay min_push_next = std::numeric_limits<delay>::max();
    delay max_push_next = std::numeric_limits<delay>::min();

    unsigned int min_ops_in_buffer{std::numeric_limits<unsigned int>::max()};
    unsigned int max_ops_in_buffer{std::numeric_limits<unsigned int>::min()};

    for (const auto &[sol, c] : generationOfSolutions) {
        const auto &ASAPST = sol.getASAPST();
        const auto &curV = dg.getVertex(c.curO);
        const auto &nextV = dg.getVertex(c.nextO);
        auto eligibleOp = curV.operation;
        delay push = ASAPST[curV.id] - ASAPTimes[curV.id];
        delay push_next = ASAPST[nextV.id] - ASAPTimes[nextV.id];

        auto iter = sol.firstPossibleOp(reEntrantMachine);
        iter++;

        unsigned int nrOps = 1;

        problem::Operation end{eligibleOp.jobId, eligibleOp.operationId + 1};

        for (; iter != sol.getMachineSequence(reEntrantMachine).end() && *iter != end; ++iter) {
            nrOps++;
        }

        min_push = std::min(min_push, push);
        max_push = std::max(max_push, push);

        min_push_next = std::min(min_push_next, push_next);
        max_push_next = std::max(max_push_next, push_next);

        min_ops_in_buffer = std::min(min_ops_in_buffer, nrOps);
        max_ops_in_buffer = std::max(max_ops_in_buffer, nrOps);
    }

    for (const auto &[sol, c] : generationOfSolutions) {
        const auto &ASAPST = sol.getASAPST();
        const auto &curV = dg.getVertex(c.curO);
        const auto &nextV = dg.getVertex(c.nextO);
        delay push = ASAPST[curV.id] - ASAPTimes[curV.id];
        delay push_next = ASAPST[nextV.id] - ASAPTimes[nextV.id];

        auto eligibleOp = curV.operation;

        auto iter = sol.firstPossibleOp(reEntrantMachine);
        iter++;

        unsigned int nrOps = 1;

        problem::Operation end{eligibleOp.jobId, eligibleOp.operationId + 1};
        for (; iter != sol.getMachineSequence(reEntrantMachine).end() && *iter != end; ++iter) {
            nrOps++;
        }

        LOG_I("Earliest current op time: {}, earliest future op time: {}, push_next: {}, nr ops "
              "committed {}",
              ASAPST[curV.id],
              ASAPST[nextV.id],
              push_next,
              nrOps);

        double push_range = (max_push != min_push) ? (max_push - min_push) : 1;
        double push_next_range =
                (max_push_next != min_push_next) ? (max_push_next - min_push_next) : 1;
        double nrOps_range = (max_ops_in_buffer != min_ops_in_buffer)
                                     ? (max_ops_in_buffer - min_ops_in_buffer)
                                     : 1;
        LOG_I("Push (norm.): {}, push_next (norm.): {}, nrOps (norm): {}",
              static_cast<double>(push - min_push) / push_range,
              static_cast<double>(push_next - min_push_next) / push_next_range,
              static_cast<double>(nrOps - min_ops_in_buffer) / nrOps_range);

        double cur_rank =
                0.75
                        * (static_cast<double>(push - min_push)
                           / push_range) // minimize effort to come to this scheduling decision
                + 0.0
                          * (static_cast<double>(push_next - min_push_next)
                             / push_next_range) // minimize effort of executing committed work
                + 0.25
                          * (static_cast<double>(nrOps - min_ops_in_buffer)
                             / nrOps_range); // maximize size of committed work

        // select the solution with lowest rank:
        ranking_heap.emplace(sol, cur_rank);
    }

    std::vector<PartialSolution> ranked;
    // output from least likely to most likely 'optimal'
    while (!ranking_heap.empty()) {
        ranked.push_back(ranking_heap.top().first);
        ranking_heap.pop();
    }
    //    std::stable_sort(ranked.begin(), ranked.end()); // use stable sort to retain the ordering
    //    of the insertion in case of ties in ranking.
    return ranked;
}

BranchBoundNode createStupidSchedule(const problem::Instance &problem,
                                     problem::MachineId reentrant_machine,
                                     delay trivialLowerBound) {
    Sequence stupidSequence;
    auto reEntrantMachine = problem.getReEntrantMachines().front();
    const auto firstPass = problem.getOperationsMappedOnMachine().at(reentrant_machine).at(0);
    const auto secondPass = problem.getOperationsMappedOnMachine().at(reentrant_machine).at(1);

    auto dg = problem.getDelayGraph();

    // Do STUPID SCHEDULING: i.e. one product at a time in the re-entrant loop
    // for each except the last job

    for (const auto jobId : problem.getJobsOutput()) {
        const auto &jobOps = problem.getJobOperationsOnMachine(jobId, reentrant_machine);
        stupidSequence.insert(stupidSequence.end(), jobOps.begin(), jobOps.end());
    }

    return BranchBoundNode(problem,
                           dg,
                           PartialSolution({{reEntrantMachine, stupidSequence}},
                                           {},
                                           {{reEntrantMachine, stupidSequence.size() - 1}}),
                           trivialLowerBound);
}

std::vector<PartialSolution> scheduleOneOperation(cg::ConstraintGraph &dg,
                                                  const problem::Instance &problem,
                                                  const PartialSolution &solution,
                                                  const cg::Vertex &eligibleOperation) {
    // copy the current solution to become the new solution
    auto reEntrantMachine = problem.getReEntrantMachines().front();
    LOG(fmt::format("Starting from current solution {}", solution));

    // create all options that are potentially feasible:
    auto [last_potentially_feasible_option, options] =
            forward::createOptions(problem, solution, eligibleOperation, reEntrantMachine);

    // update the ASAPTimes for the coming window, so that we have enough information to compute the
    // ranking
    const auto job_start = eligibleOperation.operation.jobId;
    std::vector<delay> ASAPTimes = solution.getASAPST();
    algorithms::paths::computeASAPST(
            dg,
            ASAPTimes,
            dg.getVerticesC(std::max(job_start, problem::JobId(1)) - 1),
            dg.getVerticesC(job_start, last_potentially_feasible_option.jobId));

    if (options.empty()) {
        cg::exports::saveAsTikz(problem, solution, "no_options_left.tex");
        LOG_C("No options could be made for {}", eligibleOperation.operation);
        throw FmsSchedulerException("Unable to create any option!");
    }

    LOG(std::string() + "*** nr options: " + std::to_string(options.size()));

    std::vector<std::pair<PartialSolution, SchedulingOption>> newGenerationOfSolutions =
            forward::evaluateOptionFeasibility(
                    dg, problem, solution, options, ASAPTimes, reEntrantMachine);
    if (newGenerationOfSolutions.empty()) {
        throw FmsSchedulerException("No feasible options; not possible for Canon case!");
    }
    return ranked(dg, problem, newGenerationOfSolutions, ASAPTimes);
}

} // namespace fms::solvers::branch_bound
