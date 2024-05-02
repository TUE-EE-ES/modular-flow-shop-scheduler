#include "solvers/backtrack.hpp"

#include "delayGraph/builder.hpp"
#include "delayGraph/delayGraph.h"
#include "delayGraph/export_utilities.h"
#include "fmsschedulerexception.h"
#include "solvers/forwardheuristic.h"
#include "solvers/utils.hpp"
#include "utils/sorting.hpp"
#include "utils/time.h"

#include <chrono>
#include <fmt/compile.h>
#include <stack>
#include <utility>

using namespace algorithm;
using namespace std;
using namespace FORPFSSPSD;
using namespace DelayGraph;

using BackTempSolution = std::tuple<size_t, size_t, size_t, PartialSolution>;

// Max 30 minutes of searching
const int kTimeMax = 1800000;
const char *const kModularAlgorithmOption = "fullcheck";

std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
BackTrackSolver::solve(FORPFSSPSD::ProductionLine &problemInstance, const commandLineArgs &args) {
    // // solve the instance
    // LOG("Computation of the schedule started");

    // if (!args.modularAlgorithmOption.empty()
    //     && args.modularAlgorithmOption != kModularAlgorithmOption) {
    //     LOG(LOGGER_LEVEL::WARNING, fmt::format("Unknown option: {}", args.modularAlgorithmOption));
    // }

    // // make a copy of the delaygraph
    // if (!problemInstance.isGraphInitialized()) {
    //     problemInstance.updateDelayGraph(Builder::FORPFSSPSD(problemInstance));
    // }
    // auto dg = problemInstance.getDelayGraph();

    // if (args.verbose >= LOGGER_LEVEL::DEBUG) {
    //     auto name = fmt::format("input_graph_{}.dot", problemInstance.getProblemName());
    //     DelayGraph::export_utilities::saveAsDot(dg, name);
    // }

    // auto [result, ASAPST] = SolversUtils::checkSolutionAndOutputIfFails(problemInstance);

    // LOG(fmt::format("Number of vertices in the delay graph is {}", dg.get_number_of_vertices()));

    // PartialSolution::MachineEdges initialSequence;
    // const auto &reEntrantMachines = problemInstance.getReEntrantMachines();
    // for (FORPFSSPSD::MachineId machine : reEntrantMachines) {
    //     initialSequence[machine] =
    //             ForwardHeuristic::createInitialSequence(problemInstance, machine);
    // }

    // PartialSolution solution(initialSequence, ASAPST);
    // auto lowerBound = solution.getMakespan();

    // std::stack<BackTempSolution> backTrackStack;
    // // Push the initial solution: 1st job, 2nd operation, and 1st re-entrant machine
    // backTrackStack.emplace(0, 1, 0, solution);

    // size_t maxReEntrantOps = 0;
    // for (const auto &reEntrantMachine : reEntrantMachines) {
    //     maxReEntrantOps = std::max(maxReEntrantOps,
    //                                problemInstance.getMachineOperations(reEntrantMachine).size());
    // }

    // const auto &jobs = problemInstance.getJobsOutput();
    // size_t iterations = 0;
    // auto start = std::chrono::milliseconds(getCpuTime());
    // auto maxTime = std::chrono::milliseconds(kTimeMax);
    // while (!backTrackStack.empty()) {
    //     auto [jobIdx, opIdx, machineIdx, currentSolution] = backTrackStack.top();
    //     backTrackStack.pop();

    //     const auto oldOpIdx = opIdx;
    //     const auto &job = jobs[jobIdx];
    //     const auto &machine = reEntrantMachines[machineIdx];
    //     const auto &ops = problemInstance.getMachineOperations(machine);

    //     // Check if solution is leaf
    //     ++machineIdx;
    //     if (machineIdx == reEntrantMachines.size()) {
    //         machineIdx = 0;
    //         ++opIdx;
    //         if (opIdx == ops.size()) {
    //             opIdx = 1;
    //             ++jobIdx;
    //             // The last job is already scheduled so we schedule all - 1
    //             if (jobIdx + 1 >= jobs.size()) {
    //                 // We scheduled everything
    //                 solution = std::move(currentSolution);
    //                 break;
    //             }
    //         }
    //     }

    //     std::vector<PartialSolution> newSolutions;
    //     if (oldOpIdx < ops.size()) {
    //         const auto &eligibleOperation = dg.get_vertex({job, ops[oldOpIdx]});
    //         newSolutions = getFeasibleOptionsSorted(dg,
    //                                                 problemInstance,
    //                                                 eligibleOperation,
    //                                                 currentSolution,
    //                                                 args,
    //                                                 args.modularAlgorithmOption == "fullcheck");
    //     } else {
    //         newSolutions = {currentSolution};
    //     }

    //     for (auto &newSolution : newSolutions) {
    //         backTrackStack.emplace(jobIdx, opIdx, machineIdx, std::move(newSolution));
    //     }

    //     if (getCpuTime() - start > maxTime) {
    //         LOG(LOGGER_LEVEL::WARNING, "Timeout reached. Aborting.");
    //         solution = SolversUtils::createTrivialSolution(problemInstance);
    //         return {{std::move(solution)},
    //                 {{"timeout", true}, {"iterations", iterations}, {"lowerBound", lowerBound}}};
    //     }

    //     ++iterations;
    // }

    // if (args.verbose >= LOGGER_LEVEL::DEBUG) {
    //     auto name = fmt::format("output_graph_{}.dot", problemInstance.getProblemName());
    //     DelayGraph::export_utilities::saveAsDot(problemInstance, solution, name);
    // }

    // SolversUtils::checkSolutionAndOutputIfFails(problemInstance, solution);
    // return {{std::move(solution)}, {{"iterations", iterations}, {"lowerBound", lowerBound}}};
    return {};
}

std::vector<PartialSolution> algorithm::BackTrackSolver::getFeasibleOptionsSorted(
        delayGraph &dg,
        const FORPFSSPSD::Instance &problem,
        const vertex &eligibleOperation,
        const PartialSolution &solution,
        const commandLineArgs &args,
        bool fullCheck) {

    auto [solutions, minRankId] =
            ForwardHeuristic::getFeasibleOptions(dg, problem, eligibleOperation, solution, args);

    std::vector<PartialSolution> result;
    for (auto &[solution, tmpDg] : solutions) {
        if (fullCheck) {
            auto LPresult = algorithm::LongestPath::computeASAPST(dg, solution.getAllChosenEdges());
            if (!std::get<0>(LPresult).positiveCycle.empty()) {
                continue;
            }
        }
        // TODO(joan): Maintenance uses a delay graph for each solution, we are not copying it for
        // now to avoid an explosion in memory consumption.
        result.emplace_back(std::move(solution));
    }

    // Sort the operations by their rank using tag sort
    std::vector<std::size_t> indices(result.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(),
              indices.end(),
              [&validSolutions = result](std::size_t i1, std::size_t i2) {
                  return validSolutions[i1].getRanking() > validSolutions[i2].getRanking();
              });
    algorithm::applyPermutationInPlace(result.begin(), result.end(), indices.begin());

    return result;
}
