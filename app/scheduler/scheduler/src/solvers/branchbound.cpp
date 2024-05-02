#include "solvers/branchbound.h"

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/operation.h"
#include "delayGraph/builder.hpp"
#include "delayGraph/delayGraph.h"
#include "delayGraph/export_utilities.h"
#include "longest_path.h"
#include "solvers/forwardheuristic.h"
#include "solvers/paretoheuristic.h"

#include "pch/utils.hpp"

#include <chrono>
#include <fstream>
#include <iomanip> // std::setprecision
#include <iostream>
#include <queue>

using namespace algorithm;
using namespace std;
using namespace FORPFSSPSD;
using namespace DelayGraph;

class BranchBoundNode {
    PartialSolution solution;
    delay lowerbound;
    delay makespan;
    FORPFSSPSD::operation lastInsertedOperation;
public:

    const PartialSolution & getSolution() const {
        return solution;
    }

    delay getLowerbound() const {
        return lowerbound;
    }

    delay getMakespan() const {
        return makespan;
    }

    FORPFSSPSD::operation getLastInsertedOperation() const
    {
        return lastInsertedOperation;
    }

    std::vector<delay> getASAPST(const FORPFSSPSD::Instance & problem, delayGraph & dg) const
    {
        std::vector<delay> ASAPST = LongestPath::initializeASAPST(dg);
        // determine the sequencing edges
        std::vector<edge> final_sequence =
                solution.getAllChosenEdges(); // make a copy of the chosen edges

        for(const auto& e : problem.infer_pim_edges(solution)) {
            final_sequence.push_back(e);
        }
        // compute (over the whole window) the ASAPST for these sequencing edges
        // assume it is feasible, otherwise this will take a very long time
        LongestPathResult result = ForwardHeuristic::validateInterleaving(
                dg, problem, final_sequence, ASAPST, {}, dg.cget_vertices());
        if(!result.positiveCycle.empty()) {
            std::cout << "Detected infeasible edges: " << std::endl;
            for(const auto & edge : result.positiveCycle) {
                LOG(fmt::format("-- {}\n", edge));
            }

            export_utilities::saveAsDot(dg, "inconsistent.dot", final_sequence);
            std::cout << chosenEdgesToString(solution, dg);

            throw FmsSchedulerException("Positive cycle encountered or invalid constraints encountered while determining lowerbound of partial solution");
        }

        return ASAPST;
    }

    BranchBoundNode(FORPFSSPSD::Instance & problem, delayGraph & dg, const PartialSolution& solution)
        : solution(solution) {
        this->solution.clearASAPST();
        std::vector<delay> ASAPST = getASAPST(problem, dg);
        auto reEntrantMachine = problem.getReEntrantMachines().front();

        edge e = *(solution.first_possible_edge(reEntrantMachine));
        lastInsertedOperation =
                dg.get_vertex((*solution.first_possible_edge(reEntrantMachine)).dst).operation;

        // calculate lowerbounds

        // OPTION 1:
        lowerbound = ASAPST.back(); // use the current BFM as the lower bound
        makespan = ASAPST.back();

        // OPTION 2: lower bound of scheduled part + sum of processing times for first and second passes
        // the remaining first pass operations are not scheduled yet
        // there are second pass operations that are not scheduled yet

        // add the minimal time for first passes and the minimal time for second passes
        // the minimal time for first passes needs to be determined for each type of plexity

        delay lowerbound2 = problem.getTrivialCompletionTimeLowerbound();

        // TODO: a lowerbound that can tighten dynamically when scheduling decisions are taken
        // i.e., a combination of the trivial completion time and the current scheduling result

        lowerbound = std::max(lowerbound, lowerbound2);

    }
};

bool operator>(const BranchBoundNode& lhs, const BranchBoundNode &rhs) {
    return lhs.getLowerbound() > rhs.getLowerbound();
}

PartialSolution BranchBound::solve(FORPFSSPSD::Instance &problemInstance, const commandLineArgs &args){
    // solve the instance
    LOG("Started branch and bound");

    // make a copy of the delaygraph
    if (!problemInstance.isGraphInitialized()) {
        problemInstance.updateDelayGraph(Builder::FORPFSSPSD(problemInstance));
    }
    auto dg = problemInstance.getDelayGraph();

    if (args.verbose >= LOGGER_LEVEL::DEBUG) {
        DelayGraph::export_utilities::saveAsTikz(problemInstance, dg, "input_graph.tex");
    }

    std::vector<delay> ASAPST = algorithm::LongestPath::initializeASAPST(dg);
    auto result = algorithm::LongestPath::computeASAPST(dg, ASAPST);

    // check wether the input graph is feasible or not
    if(!result.positiveCycle.empty())
    {
        LOG(LOGGER_LEVEL::FATAL, "The input graph is infeasible. Aborting.");
        throw FmsSchedulerException("The input graph is infeasible. Aborting.");
    }

    LOG(std::string() + "Number of vertices in the delay graph is " + std::to_string(dg.get_number_of_vertices()));

    // find out which machine is the re-entrant machine
    MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();
    const std::vector<unsigned int> & ops = problemInstance.getOperationsMappedOnMachine().at(reentrant_machine); // check how many operations are mapped

    std::vector<edge> initial_sequence =
            ForwardHeuristic::createInitialSequence(problemInstance, reentrant_machine);

    // best-first can be implemented by a priority_queue
//    std::priority_queue<BranchBoundNode, std::vector<BranchBoundNode>, std::greater<BranchBoundNode>> open_nodes; // use a min-heap to retrieve the solution with the smallest lower bound
    // LIFO (depth-first)
    std::vector<BranchBoundNode> open_nodes; // use a 'stack' to continue with the last added node first
    open_nodes.emplace_back(
            problemInstance, dg, PartialSolution({{reentrant_machine, initial_sequence}}, ASAPST));

    LOGGER_LEVEL old_level = Logger::getVerbosity();
    LOG(LOGGER_LEVEL::INFO, "Using INITIAL SCHEDULING to get initial result");
    Logger::setVerbosity(LOGGER_LEVEL::FATAL);

    // The Branch & Bound algorithm can be seeded by creating any initial schedule (e.g., the 'stupid schedule', a bhcs/md-bhcs result)
    BranchBoundNode stupidScheduleNode(createStupidSchedule(problemInstance, reentrant_machine));
    BranchBoundNode bhcsNode(problemInstance, dg, ForwardHeuristic::solve(problemInstance, args));
    LOG(LOGGER_LEVEL::FATAL, std::string() + "Seed with BHCS completed with makespan of " + std::to_string(bhcsNode.getMakespan()));

    // The Branch & Bound algorithm is seeded with the best result from the Pareto scheduler
    commandLineArgs argss = args;
    argss.maxPartialSolutions = 20;
    std::vector<PartialSolution> solutions = ParetoHeuristic::solve(problemInstance, argss);
    PartialSolution best = solutions.back();
    for(const PartialSolution & sol : solutions) {
        if(best.getMakespan() > sol.getMakespan()) {
            best = sol;
        }
    }
    LOG(LOGGER_LEVEL::FATAL, std::string() + "Seed with MD-BHCS completed with makespan of " + std::to_string(best.getMakespan()));

    BranchBoundNode mdbhcsNode(problemInstance, dg, best);

    BranchBoundNode bestFoundNode = mdbhcsNode; // Choose which algorithm's result to seed with
    if(bestFoundNode.getMakespan() > bhcsNode.getMakespan()) {
        bestFoundNode = bhcsNode;
    }
    if(bestFoundNode.getMakespan() < open_nodes.back().getLowerbound()) {
        LOG(LOGGER_LEVEL::FATAL, std::to_string(bestFoundNode.getMakespan()) + " is smaller than initial lowerbound " + std::to_string(open_nodes.back().getLowerbound()));
        throw FmsSchedulerException("Either the initial lowerbound or the initial solution is incorrect; found a (valid?) solution that is lower than the initial lower bound");
    }
    Logger::setVerbosity(old_level);
    LOG(LOGGER_LEVEL::FATAL, "Finished INITIAL SCHEDULING heuristic with makespan " + std::to_string(bestFoundNode.getMakespan()));

    auto start = FMS::getCpuTime();

    delay previous_iteration_lowerbound = 0;

    unsigned int iteration = 0;
    unsigned int retired = 0;
    while(!open_nodes.empty())
    {
        delay lowerbound = bestFoundNode.getMakespan();
        for(const BranchBoundNode & n : open_nodes) {
            lowerbound = std::min(lowerbound, n.getLowerbound());
        }

        LOG(LOGGER_LEVEL::INFO, std::string() + "Open nodes: " + std::to_string(open_nodes.size()));
        const BranchBoundNode node = open_nodes.back();
        open_nodes.pop_back();

        PartialSolution solution = node.getSolution();
        // recompute the solution ASAPST every time to save a lot of memory (feasible sequences are cheap to compute anyway)
        solution.setASAPST(node.getASAPST(problemInstance, dg));

        // sanity check on the lowerbound values calculated. Exploring the solution space further
        // must only lead to stricter lowerbounds, never looser ones
        if(previous_iteration_lowerbound > lowerbound) {
            throw FmsSchedulerException(std::string() + "Lower bound decreased! This cannot happen with a proper lower bound!"
                                        + std::to_string(previous_iteration_lowerbound) + " > " + std::to_string(lowerbound));
        } else {
            if(previous_iteration_lowerbound != lowerbound) { // write to a file, the lowerbound has been changed
                std::ofstream stream(args.outputFile + ".lb");
                stream << std::min(lowerbound, bestFoundNode.getMakespan());
                stream.close();
            }
            previous_iteration_lowerbound = lowerbound;
        }

        if(lowerbound >= bestFoundNode.getMakespan()) {
            // found an optimal solution, finish execution!
            LOG(LOGGER_LEVEL::FATAL, "Optimal solution found");
            return PartialSolution(bestFoundNode.getSolution().getChosenEdgesPerMachine(),
                                   bestFoundNode.getASAPST(problemInstance, dg));
        }
        if((iteration % 800) == 0) {
            std::cout
                    << std::setw(12) << "ITERATION"
                    << std::setw(15) << "LOWERBOUND"
                    << std::setw(15) << "BEST FOUND"
                    << std::setw(12) << "GAP (%)"
                    << std::setw(12) << "NODES LEFT"
                    << std::setw(16) << "NODES RETIRED"
                    << std::setw(18) << "TIME SPENT (s)"
                    << std::setw(22) << "TIME SPENT/NODE (s)"
                    << std::endl;
        }

        if(((iteration++)+1) % 40 == 0) {
            auto time_spent = FMS::getCpuTime() - start;
            auto precision = std::cout.precision(4);

            std::cout
                      << std::setw(12) << iteration
                      << std::setw(15) << lowerbound
                      << std::setw(15) << bestFoundNode.getMakespan()
                      << std::setw(12) << (((double) bestFoundNode.getMakespan()-lowerbound)/(double)lowerbound) * 100
                      << std::setw(12) << open_nodes.size()
                      << std::setw(16) << retired
                      << std::setw(18) << time_spent
                      << std::setw(22) << time_spent/iteration
                      << std::endl;

            std::cout << std::setprecision(precision);
            if (time_spent > args.timeOut) {
                LOG(LOGGER_LEVEL::FATAL, "Time limit exceeded");
                return {bestFoundNode.getSolution().getChosenEdgesPerMachine(),
                        bestFoundNode.getASAPST(problemInstance, dg)};
            }
        }

        if (bestFoundNode.getASAPST(problemInstance, dg).at(dg.get_vertices().back().id)
            <= node.getLowerbound()) {
            // ignore this branch, as it can never become optimal
            retired ++;
            continue;
        }

        bool scheduled_one_operation = false;
        // schedule one eligible node
        for(unsigned int i = 0; i < problemInstance.getNumberOfJobs() - 1; i++) {
            if(scheduled_one_operation) break;

            bool first = true; // First operation is already included in the initial sequence
            for(unsigned int op : ops) {
                if(!first) {
                    JobId firstPossibleJob =
                            dg.get_vertex((*solution.first_possible_edge(reentrant_machine)).src)
                                    .operation.jobId;
                    if (dg.is_source((*solution.first_possible_edge(reentrant_machine)).src)
                        || i > firstPossibleJob) {
                        // schedule the next operation
                        const auto &eligibleOperation = dg.get_vertex({i, op});
                        auto newSolutions = scheduleOneOperation(dg, problemInstance, solution, eligibleOperation);

                         if(i + 2 == problemInstance.getNumberOfJobs()) { // if it was the (second-to-)last sheet to schedule (last second pass is already included)
                            for(const auto& s : newSolutions) {
                                BranchBoundNode new_node(problemInstance, dg, s);
                                if(bestFoundNode.getMakespan() > new_node.getMakespan()) {
                                    LOG(LOGGER_LEVEL::WARNING, std::string() + "Found a better solution " + std::to_string(new_node.getMakespan()) +" is smaller than " + std::to_string(bestFoundNode.getMakespan()));

                                    bestFoundNode = new_node;
                                }
                            }
                        } else {
                            // add each of the feasible solutions to the heap of open nodes
                            LOG(LOGGER_LEVEL::INFO, std::string() + "Adding " + std::to_string(newSolutions.size()) + " nodes");

                            // for the stack-based branch and bound the order of insertion makes a big difference, the one that is
                            // more likely to be optimal should be evaluated earlier
                            for(PartialSolution & s : newSolutions) {
                                BranchBoundNode newNode(problemInstance, dg, s);
                                std::string edges1 = chosenEdgesToString(solution, dg);
                                std::string edges2 = chosenEdgesToString(s, dg);

                                if(newNode.getLowerbound() < node.getLowerbound()) {
                                    LOG(LOGGER_LEVEL::FATAL, "Lower bound decreased by inserting an operation!");
                                    LOG(LOGGER_LEVEL::FATAL,
                                        fmt::format("{} -> {}",
                                                    dg.get_vertex((*s.first_possible_edge(
                                                                           reentrant_machine))
                                                                          .src)
                                                            .operation,
                                                    dg.get_vertex((*s.first_possible_edge(
                                                                           reentrant_machine))
                                                                          .dst)
                                                            .operation));
                                    LOG(LOGGER_LEVEL::INFO, "original node: "+ edges1 + ": " + std::to_string(node.getLowerbound()));
                                    LOG(LOGGER_LEVEL::INFO, "new node: " + edges2 + ": " + std::to_string(newNode.getLowerbound()));

                                    std::ofstream before("before_insertion.txt");
                                    before << edges1;
                                    before.close();
                                    std::ofstream after("after_insertion.txt");
                                    after << edges2;
                                    after.close();

                                    throw FmsSchedulerException("Lower bound decreased by making a scheduling decision! This cannot happen with a proper lower bound!");
                                }

                                if(newNode.getLowerbound() < bestFoundNode.getMakespan())
                                {
                                    open_nodes.push_back(newNode);
                                } else {
                                    retired ++;
                                }
                            }
                        }
                        scheduled_one_operation = true;
                        break; // Break out of the loop after scheduling the current operation, continue with another Branch and Bound node
                    }
                }
                first = false;
            }
        }
    }

    if(args.verbose >= LOGGER_LEVEL::DEBUG)
        DelayGraph::export_utilities::saveAsTikz(problemInstance, dg, "output_graph.tex");

    std::ofstream stream(args.outputFile + ".lb");
    stream << bestFoundNode.getMakespan();
    stream.close();

    LOG(LOGGER_LEVEL::FATAL, "Optimal solution found (no more branches left to explore)");
    return PartialSolution(bestFoundNode.getSolution().getChosenEdgesPerMachine(),
                           bestFoundNode.getASAPST(problemInstance, dg));
}

std::vector<PartialSolution>
BranchBound::ranked(delayGraph &dg,
                    const FORPFSSPSD::Instance &problemInstance,
                    const std::vector<std::pair<PartialSolution, option>> &generationOfSolutions,
                    const std::vector<delay> &ASAPTimes) {
    using ranked_solution = std::pair<PartialSolution, double>;
    auto comp = []( const ranked_solution& a, const ranked_solution& b ) { return a.second < b.second; }; // rank highest to lowest
    std::priority_queue<ranked_solution, std::vector<ranked_solution>, decltype( comp )> ranking_heap(comp);

    auto reEntrantMachine = problemInstance.getReEntrantMachines().front();

    // rank all options, normalized
    delay min_push = std::numeric_limits<delay>::max();
    delay max_push = std::numeric_limits<delay>::min();
    delay min_push_next = std::numeric_limits<delay>::max();
    delay max_push_next = std::numeric_limits<delay>::min();

    unsigned int min_ops_in_buffer{std::numeric_limits<unsigned int>::max()};
    unsigned int max_ops_in_buffer{std::numeric_limits<unsigned int>::min()};

    for(auto & p : generationOfSolutions) {
        auto & sol = p.first;
        const auto & ASAPST = sol.getASAPST();
        const auto & c = p.second;
        const auto &curV = dg.get_vertex(c.curV);
        const auto &nextV = dg.get_vertex(c.nextV);
        auto eligibleOp = curV.operation;
        delay push = ASAPST[curV.id] - ASAPTimes[curV.id];
        delay push_next = ASAPST[nextV.id] - ASAPTimes[nextV.id];

        auto iter = sol.first_possible_edge(reEntrantMachine);
        iter++;

        unsigned int nrOps = 1;

        FORPFSSPSD::operation end {eligibleOp.jobId, eligibleOp.operationId+1};

        for (; iter != sol.getChosenEdges(reEntrantMachine).end()
               && dg.get_vertex((*iter).src).operation != end;
             ++iter) {
            nrOps++;
        }

        min_push = std::min(min_push, push);
        max_push = std::max(max_push, push);

        min_push_next = std::min(min_push_next, push_next);
        max_push_next = std::max(max_push_next, push_next);

        min_ops_in_buffer = std::min(min_ops_in_buffer, nrOps);
        max_ops_in_buffer = std::max(max_ops_in_buffer, nrOps);
    }

    for(auto & p : generationOfSolutions) {
        auto & sol = p.first;
        const auto & ASAPST = sol.getASAPST();
        const auto & c = p.second;
        const auto &curV = dg.get_vertex(c.curV);
        const auto &nextV = dg.get_vertex(c.nextV);
        delay push = ASAPST[curV.id] - ASAPTimes[curV.id];
        delay push_next = ASAPST[nextV.id] - ASAPTimes[nextV.id];

        auto eligibleOp = curV.operation;

        auto iter = sol.first_possible_edge(reEntrantMachine);
        iter++;

        unsigned int nrOps = 1;

        FORPFSSPSD::operation end {eligibleOp.jobId, eligibleOp.operationId+1};
        for (; iter != sol.getChosenEdges(reEntrantMachine).end()
               && dg.get_vertex((*iter).src).operation != end;
             ++iter) {
            nrOps++;
        }

        LOG(LOGGER_LEVEL::INFO,
            std::string() + "Earliest current op time: " + std::to_string(ASAPST[c.curV])
                    + ", earliest future op time: " + std::to_string(ASAPST[c.nextV])
                    + ", push_next: " + std::to_string(push_next) + ", nr ops committed "
                    + std::to_string(nrOps));

        double push_range = (max_push != min_push) ? (max_push - min_push) : 1;
        double push_next_range = (max_push_next != min_push_next) ? (max_push_next - min_push_next) : 1;
        double nrOps_range = (max_ops_in_buffer != min_ops_in_buffer) ? (max_ops_in_buffer - min_ops_in_buffer) : 1;
        LOG(LOGGER_LEVEL::INFO, std::string() + "Push (norm.): " + std::to_string((double)(push-min_push) / push_range) + ", push_next (norm.): " + std::to_string((double)(push_next -min_push_next) / push_next_range) + ", nrOps (norm): " + std::to_string((double)((nrOps - min_ops_in_buffer)/nrOps_range)));

        double cur_rank =
                0.75 * ((double)(push-min_push) / push_range) // minimize effort to come to this scheduling decision
                + 0.0 * ((double)(push_next -min_push_next) / push_next_range) // minimize effort of executing committed work
                + 0.25 * ((double)(nrOps - min_ops_in_buffer) / nrOps_range); // maximize size of committed work

        // select the solution with lowest rank:
        ranking_heap.push({sol, cur_rank});
    }

    std::vector<PartialSolution> ranked;
    // output from least likely to most likely 'optimal'
    while(!ranking_heap.empty()) {
        ranked.push_back(ranking_heap.top().first);
        ranking_heap.pop();
    }
//    std::stable_sort(ranked.begin(), ranked.end()); // use stable sort to retain the ordering of the insertion in case of ties in ranking.
    return ranked;
}

BranchBoundNode BranchBound::createStupidSchedule(FORPFSSPSD::Instance &problemInstance , MachineId reentrant_machine)
{
    Edges stupid_sequence;
    auto reEntrantMachine = problemInstance.getReEntrantMachines().front();
    unsigned int first_pass = problemInstance.getOperationsMappedOnMachine().at(reentrant_machine).at(0);
    unsigned int second_pass = problemInstance.getOperationsMappedOnMachine().at(reentrant_machine).at(1);

    auto dg = problemInstance.getDelayGraph();

    // Do STUPID SCHEDULING: i.e. one product at a time in the re-entrant loop
    for(unsigned int i = 0; i < problemInstance.getNumberOfJobs() - 1; i++ ) { // for each except the last job

        if(problemInstance.getPlexity(i) == FORPFSSPSD::Plexity::SIMPLEX) {
            if(problemInstance.getPlexity(i+1) == FORPFSSPSD::Plexity::SIMPLEX) {
                // SIMPLEX - SIMPLEX
                const auto &src = dg.get_vertex({i, second_pass});
                const auto &dst = dg.get_vertex({i + 1, second_pass});
                stupid_sequence.push_back(edge(src.id, dst.id, problemInstance.query(src, dst)));

            } else { // i+1 is DUPLEX
                // SIMPLEX - DUPLEX
                const auto &src = dg.get_vertex({i, second_pass});
                const auto &dst = dg.get_vertex({i + 1, first_pass});
                stupid_sequence.push_back(edge(src.id, dst.id, problemInstance.query(src, dst)));
            }
        } else { // i is DUPLEX
            const auto &src = dg.get_vertex({i, first_pass});
            const auto &dst = dg.get_vertex({i, second_pass});
            stupid_sequence.push_back(edge(src.id, dst.id, problemInstance.query(src, dst)));

            if(problemInstance.getPlexity(i+1) == FORPFSSPSD::Plexity::SIMPLEX) {
                // DUPLEX - SIMPLEX
                const auto &src = dg.get_vertex({i, second_pass});
                const auto &dst = dg.get_vertex({i + 1, second_pass});
                stupid_sequence.push_back(edge(src.id, dst.id, problemInstance.query(src, dst)));
            } else { // i+1 is DUPLEX
                // DUPLEX - DUPLEX

                const auto &src = dg.get_vertex({i, second_pass});
                const auto &dst = dg.get_vertex({i + 1, first_pass});
                stupid_sequence.push_back(edge(src.id, dst.id, problemInstance.query(src, dst)));
            }
        }
    }
    return BranchBoundNode(problemInstance,
                           dg,
                           PartialSolution({{reEntrantMachine, stupid_sequence}},
                                           {},
                                           {{reEntrantMachine, stupid_sequence.size() - 1}}));
}

std::vector<PartialSolution>
BranchBound::scheduleOneOperation(delayGraph &dg,
                                  const FORPFSSPSD::Instance &problem,
                                  const PartialSolution &solution,
                                  const vertex &eligibleOperation) {
    // copy the current solution to become the new solution
    auto reEntrantMachine = problem.getReEntrantMachines().front();
    LOG(fmt::format("Starting from current solution {}", solution));

#if 0
    std::cout << "beginning of iteration (after reduce):"<< std::endl;
    for(auto some : currentGeneration) {
        std::cout << some << std::endl;
    }
#endif
    // create all options that are potentially feasible:
    auto [last_potentially_feasible_option, options] = ForwardHeuristic::createOptions(
            dg,problem, solution, eligibleOperation, reEntrantMachine);

    // update the ASAPTimes for the coming window, so that we have enough information to compute the ranking
    const unsigned int job_start = eligibleOperation.operation.jobId;
    std::vector<delay> ASAPTimes = solution.getASAPST();
    LongestPath::computeASAPST(
            dg,
            ASAPTimes,
            dg.cget_vertices(max(job_start, 1u) - 1),
            dg.cget_vertices(job_start,
                             dg.get_vertex(last_potentially_feasible_option.dst).operation.jobId));

    if(options.empty()) {
        export_utilities::saveAsTikz(problem, solution, "no_options_left.tex");
        LOG(LOGGER_LEVEL::FATAL,
            fmt::format("No options could be made for {}", eligibleOperation.operation));
        throw FmsSchedulerException("Unable to create any option!");
    }

    LOG(std::string() + "*** nr options: " + std::to_string(options.size()));

    std::vector<std::pair<PartialSolution, option>> newGenerationOfSolutions =
            ForwardHeuristic::evaluate_option_feasibility(
                    dg, problem, solution, options, ASAPTimes, reEntrantMachine);
    if(newGenerationOfSolutions.empty()) {
        throw FmsSchedulerException("No feasible options; not possible for Canon case!");
    }
    return ranked(dg, problem, newGenerationOfSolutions, ASAPTimes);
}
