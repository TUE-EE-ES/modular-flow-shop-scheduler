#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/solvers/pareto_heuristic.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/builder.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/export_utilities.hpp"
#include "fms/solvers/environmental_selection_operator.hpp"
#include "fms/solvers/forward_heuristic.hpp"
#include "fms/solvers/pareto_cull.hpp"
#include "fms/solvers/utils.hpp"

#include "fms/pch/utils.hpp"

#include <chrono>

using namespace fms;
using namespace fms::solvers;

std::vector<PartialSolution> ParetoHeuristic::solve(problem::Instance &problemInstance,
                                                    const cli::CLIArgs &args) {
    // solve the instance
    LOG("Computation of the schedule started");

    // make a copy of the delaygraph
    if (!problemInstance.isGraphInitialized()) {
        problemInstance.updateDelayGraph(cg::Builder::FORPFSSPSD(problemInstance));
    }
    auto dg = problemInstance.getDelayGraph();

    if (IS_LOG_D()) {
        auto name = fmt::format("input_graph_{}.tex", problemInstance.getProblemName());
        cg::exports::saveAsTikz(problemInstance, dg, name);
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

    const std::vector<unsigned int> &ops = problemInstance.getOperationsMappedOnMachine().at(
            reentrant_machine); // check how many operations are mapped

    std::vector<PartialSolution> solutions;
    solutions.push_back(solution);

    // iteratively schedule eligible nodes (insert higher passes between the existing sequence)
    for (unsigned int i = 0; i < problemInstance.getNumberOfJobs() - 1; i++) {
        bool first = true; // First operation is already included in the initial sequence
        for (unsigned int op : ops) {
            if (!first) {
                const auto &eligibleOperation = dg.getVertex({problem::JobId(i), op});
                solutions = scheduleOneOperation(dg,
                                                 problemInstance,
                                                 solutions,
                                                 eligibleOperation,
                                                 args.maxPartialSolutions);
            }
            first = false;
        }
    }

    if (IS_LOG_D()) {
        auto name = fmt::format("output_graph_{}.tex", problemInstance.getProblemName());
        cg::exports::saveAsTikz(problemInstance, dg, name);
    }
    return solutions;
}

delay ParetoHeuristic::determine_smallest_deadline(const cg::Vertex &v) {
    delay currentDeadline =
            std::numeric_limits<delay>::max(); // equivalent to the maximal permissible deadline
    for (const auto &[dst, weight] : v.getOutgoingEdges()) {
        if (weight < 0) {
            currentDeadline = std::min(currentDeadline, -weight);
        }
    }
    return currentDeadline;
}

std::vector<PartialSolution>
ParetoHeuristic::scheduleOneOperation(cg::ConstraintGraph &dg,
                                      const problem::Instance &problem,
                                      const std::vector<PartialSolution> &currentSolutions,
                                      const cg::Vertex &eligibleOperation,
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

    if (currentGeneration.empty()) {
        throw FmsSchedulerException("No solutions to continue with!");
    }

    for (const PartialSolution &solution : currentGeneration) {
        LOG(fmt::format(FMT_COMPILE("Starting from current_solution {}"), solution));

        if (IS_LOG_I()) {
            LOG_I("beginning of iteration (after reduce):");
            for (const auto &some : currentGeneration) {
                LOG(fmt::to_string(some));
            }
        }
        // create all option that are potentially feasible:
        auto [last_potentially_feasible_option, options] =
                forward::createOptions(problem, solution, eligibleOperation, reEntrantMachine);

        // update the ASAPTimes for the coming window, so that we have enough information to compute
        // the ranking
        const auto jobStart = eligibleOperation.operation.jobId;
        auto ASAPTimes = solution.getASAPST();

        algorithms::paths::computeASAPST(
                dg,
                ASAPTimes,
                dg.getVerticesC(std::max(jobStart, problem::JobId(1)) - 1),
                dg.getVerticesC(jobStart, last_potentially_feasible_option.jobId));

        if (options.empty()) {
            cg::exports::saveAsTikz(problem, solution, "no_options_left.tex");
            throw FmsSchedulerException("Unable to create any option!");
        }

        LOG_D(FMT_COMPILE("*** nr options: {}"), options.size());

        std::vector<std::pair<PartialSolution, SchedulingOption>> newSolutions =
                forward::evaluateOptionFeasibility(
                        dg, problem, solution, options, ASAPTimes, reEntrantMachine);

        for (const auto &sol : newSolutions) {
            newGenerationOfSolutions.push_back(sol.first);
        }
    }
    LOG(fmt::format(FMT_COMPILE("-- Size: {} became {}/{}\n"),
                    currentGeneration.size(),
                    newGenerationOfSolutions.size(),
                    pareto::simple_cull(newGenerationOfSolutions).size()));

    if (newGenerationOfSolutions.empty()) // none of the solutions were feasible...
    {
        unsigned int k = 0;
        for (auto &ps : currentGeneration) {
            auto ASAPST = ps.getASAPST(); // create a local copy that we can modify without issues
            auto result = forward::validateInterleaving(
                    dg,
                    problem,
                    ps.getChosenEdges(problem.getReEntrantMachines().front(), problem),
                    ASAPST,
                    {dg.getVertex(problem::Operation{problem::JobId(0), 0})},
                    dg.getVerticesC());
            cg::exports::saveAsTikz(
                    problem, ps, fmt::format("infeasible{}.tex", k), result.positiveCycle);
        }
        LOG_I("No feasible option has been detected for operation {}", eligibleOperation.operation);
        throw FmsSchedulerException(fmt::format("No feasible option has been found for operation "
                                                "{}. This is not possible in the Canon case",
                                                eligibleOperation.operation));
    }
    currentGeneration = pareto::simple_cull(newGenerationOfSolutions);

    a_clock::time_point end = a_clock::now();
    LOG_I(FMT_COMPILE("Scheduled operation {} in {} ms"),
          eligibleOperation.operation,
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
    return currentGeneration;
}
