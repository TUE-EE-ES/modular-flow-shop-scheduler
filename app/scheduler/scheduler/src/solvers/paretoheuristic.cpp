#include "solvers/paretoheuristic.h"

#include "delayGraph/builder.hpp"
#include "delayGraph/delayGraph.h"
#include "delayGraph/export_utilities.h"
#include "environmentalselectionoperator.h"
#include "longest_path.h"
#include "paretocull.h"
#include "solvers/forwardheuristic.h"
#include "solvers/utils.hpp"

#include "pch/utils.hpp"

#include <chrono>
#include <fmt/compile.h>

using namespace algorithm;
using namespace std;
using namespace DelayGraph;

std::vector<PartialSolution> ParetoHeuristic::solve(FORPFSSPSD::Instance &problemInstance, const commandLineArgs &args){
    // solve the instance
    LOG("Computation of the schedule started");

    // make a copy of the delaygraph
    if (!problemInstance.isGraphInitialized()) {
        problemInstance.updateDelayGraph(DelayGraph::Builder::FORPFSSPSD(problemInstance));
    }
    auto dg = problemInstance.getDelayGraph();

    if(args.verbose >= LOGGER_LEVEL::DEBUG) {
        auto name = fmt::format("input_graph_{}.tex", problemInstance.getProblemName());
        DelayGraph::export_utilities::saveAsTikz(problemInstance, dg, name);
    }

    auto [result, ASAPST] = SolversUtils::checkSolutionAndOutputIfFails(problemInstance);

    LOG(fmt::format("Number of vertices in the delay graph is {}", dg.get_number_of_vertices()));

    // We only support a single re-entrant machine in the system so choose the first one
    FORPFSSPSD::MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();
    if (problemInstance.getMachineOperations(reentrant_machine).size() > 2) {
        throw std::runtime_error("Multiple re-entrancies not implemented yet");
    }

    DelayGraph::Edges initial_sequence =
            ForwardHeuristic::createInitialSequence(problemInstance, reentrant_machine);

    PartialSolution solution({{reentrant_machine, initial_sequence}}, ASAPST);

    const std::vector<unsigned int> & ops = problemInstance.getOperationsMappedOnMachine().at(reentrant_machine); // check how many operations are mapped

    std::vector<PartialSolution> solutions;
    solutions.push_back(solution);

    // iteratively schedule eligible nodes (insert higher passes between the existing sequence)
    for(unsigned int i = 0; i < problemInstance.getNumberOfJobs() - 1; i++) {
        bool first = true; // First operation is already included in the initial sequence
        for(unsigned int op : ops) {
            if(!first) {
                const auto &eligibleOperation = dg.get_vertex({i, op});
                solutions = scheduleOneOperation(dg, problemInstance, solutions, eligibleOperation, args.maxPartialSolutions);
            }
            first = false;
        }
    }

    if(args.verbose >= LOGGER_LEVEL::DEBUG) {
        auto name = fmt::format("output_graph_{}.tex", problemInstance.getProblemName());  
        DelayGraph::export_utilities::saveAsTikz(problemInstance, dg, name);
    }
    return solutions;
}


delay ParetoHeuristic::determine_smallest_deadline(const vertex & v) {
    delay currentDeadline = std::numeric_limits<delay>::max(); // equivalent to the maximal permissible deadline
    for (const auto &[dst, weight] : v.get_outgoing_edges()) {
        if(weight < 0) {
            currentDeadline = std::min(currentDeadline, -weight);
        }
    }
    return currentDeadline;
}

std::vector<PartialSolution>
ParetoHeuristic::scheduleOneOperation(delayGraph &dg,
                                      const FORPFSSPSD::Instance &problem,
                                      const vector<PartialSolution> &currentSolutions,
                                      const vertex &eligibleOperation,
                                      const unsigned int maximumPartialSolutions) {
    using a_clock = std::chrono::steady_clock;
    a_clock::time_point start = a_clock::now();
    auto reEntrantMachine = problem.getReEntrantMachines().front();

    // at the beginning of each stage, make sure that we don't have
    // more than maxPatialSolutions in our solution set
    EnvironmentalSelectionOperator reducer(maximumPartialSolutions);
    std::vector<PartialSolution> currentGeneration = currentSolutions;
    currentGeneration = reducer.reduce(currentGeneration);

    std::vector<PartialSolution> newGenerationOfSolutions;

    if(currentGeneration.empty()) throw FmsSchedulerException("No solutions to continue with!");

    for(const PartialSolution& solution : currentGeneration) {
        LOG(fmt::format(FMT_COMPILE("Starting from current_solution {}"), solution));

        if(Logger::getVerbosity() >= LOGGER_LEVEL::INFO) {
            LOG(LOGGER_LEVEL::INFO, "beginning of iteration (after reduce):");
            for(const auto &some : currentGeneration) {
                LOG(fmt::to_string(some));
            }
        }
        // create all option that are potentially feasible:
        auto [last_potentially_feasible_option, options] = ForwardHeuristic::createOptions(dg,problem, solution, eligibleOperation, reEntrantMachine);

        // update the ASAPTimes for the coming window, so that we have enough information to compute the ranking
        const unsigned int job_start = eligibleOperation.operation.jobId;
        vector<delay> ASAPTimes = solution.getASAPST();

        LongestPath::computeASAPST(
                dg,
                ASAPTimes,
                dg.cget_vertices(max(job_start, 1u) - 1),
                dg.cget_vertices(
                        job_start,
                        dg.get_vertex(last_potentially_feasible_option.dst).operation.jobId));

        if(options.empty()) {
            export_utilities::saveAsTikz(problem, solution, "no_options_left.tex");
            throw FmsSchedulerException("Unable to create any option!");
        }

        LOG(LOGGER_LEVEL::DEBUG, fmt::format(FMT_COMPILE("*** nr options: {}"), options.size()));

        std::vector<std::pair<PartialSolution, option>> newSolutions = ForwardHeuristic::evaluate_option_feasibility(dg, problem, solution, options, ASAPTimes, reEntrantMachine);

        for(const auto& sol : newSolutions) {
            newGenerationOfSolutions.push_back(sol.first);
        }

    }
    LOG(fmt::format(FMT_COMPILE("-- Size: {} became {}/{}\n"),
                    currentGeneration.size(),
                    newGenerationOfSolutions.size(),
                    algorithm::pareto::simple_cull(newGenerationOfSolutions).size()));

    if(newGenerationOfSolutions.empty()) // none of the solutions were feasible...
    {
        unsigned int k = 0;
        for(auto & ps : currentGeneration) {
            vector<delay> ASAPST = ps.getASAPST(); // create a local copy that we can modify without issues
            auto result = ForwardHeuristic::validateInterleaving(
                    dg,
                    problem,
                    ps.getChosenEdges(problem.getReEntrantMachines().front()),
                    ASAPST,
                    {dg.get_vertex(FORPFSSPSD::operation{0, 0})},
                    dg.cget_vertices());
            export_utilities::saveAsTikz(problem, ps, std::string() + "infeasible" + std::to_string(k) + ".tex", result.positiveCycle);
        }
        LOG(LOGGER_LEVEL::INFO,
            fmt::format("No feasible option has been detected for operation {}",
                        eligibleOperation.operation));
        throw FmsSchedulerException(fmt::format("No feasible option has been found for operation "
                                                "{}. This is not possible in the Canon case",
                                                eligibleOperation.operation));
    }
    currentGeneration = algorithm::pareto::simple_cull(newGenerationOfSolutions);

    a_clock::time_point end = a_clock::now();
    LOG(LOGGER_LEVEL::INFO,
        fmt::format(FMT_COMPILE("Scheduled operation {} in {} ms"),
                    eligibleOperation.operation,
                    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()));
    return currentGeneration;
}
