#include "solvers/utils.hpp"

#include "delayGraph/delayGraph.h"
#include "delayGraph/edge.h"
#include "delayGraph/export_utilities.h"
#include "longest_path.h"

#include <fmt/compile.h>

using namespace algorithm;

std::tuple<LongestPathResult, PathTimes>
SolversUtils::checkSolutionAndOutputIfFails(const std::string &fileName,
                                            DelayGraph::delayGraph &dg,
                                            const DelayGraph::Edges &extraEdges,
                                            const std::string &extraMessage) {
    auto result = algorithm::LongestPath::computeASAPST(dg, extraEdges);
    checkPathResultAndOutputIfFails(fileName, dg, extraMessage, std::get<0>(result), extraEdges);
    return std::move(result);
}

std::tuple<LongestPathResult, PathTimes>
algorithm::SolversUtils::checkSolutionAndOutputIfFails(const FORPFSSPSD::Instance &instance) {
    const auto &dg = instance.getDelayGraph();
    auto result = algorithm::LongestPath::computeASAPST(dg);
    
    checkPathResultAndOutputIfFails(
            fmt::format(FMT_COMPILE("input_infeasible_{}"), instance.getProblemName()),
            dg,
            "The input graph is infeasible, aborting.",
            std::get<0>(result));

    return std::move(result);
}

std::tuple<LongestPathResult, PathTimes>
algorithm::SolversUtils::checkSolutionAndOutputIfFails(const FORPFSSPSD::Instance &instance,
                                                       PartialSolution &ps) {
    auto dg = instance.getDelayGraph();
    return checkSolutionAndOutputIfFails(
            fmt::format(FMT_COMPILE("output_infeasible_{}"), instance.getProblemName()),
            dg,
            ps.getAllChosenEdges(),
            "The created solution is infeasible!");
}

void algorithm::SolversUtils::checkPathResultAndOutputIfFails(
        const std::string &fileName,
        const DelayGraph::delayGraph &dg,
        const std::string &extraMessage,
        const algorithm::LongestPathResult &result,
        const DelayGraph::Edges &extraEdges) {
    if (!result.positiveCycle.empty()) {
        // Save the file as DOT
        auto name = fileName + ".dot";
        DelayGraph::export_utilities::saveAsDot(dg, name, extraEdges, result.positiveCycle);
        throw FmsSchedulerException(extraMessage);
    }
}

PartialSolution algorithm::SolversUtils::createTrivialSolution(FORPFSSPSD::Instance &problem) {
    const auto &reEntrantMachines = problem.getReEntrantMachines();
    PartialSolution::MachineEdges edges;
    DelayGraph::Edges allEdges;

    for (const auto &machine : reEntrantMachines) {
        auto machineEdges = createMachineTrivialSolution(problem, machine);
        allEdges.insert(allEdges.end(), machineEdges.begin(), machineEdges.end());
        edges.emplace(machine, std::move(machineEdges));
    }

    DelayGraph::delayGraph dg = problem.getDelayGraph();
    auto [result, ASAPST] = algorithm::LongestPath::computeASAPST(dg, allEdges);
    checkPathResultAndOutputIfFails(
            fmt::format(FMT_COMPILE("output_infeasible_{}"), problem.getProblemName()),
            dg,
            "The created trivial solution is infeasible!",
            result);
    return {edges, std::move(ASAPST)};
}

DelayGraph::Edges
algorithm::SolversUtils::createMachineTrivialSolution(const FORPFSSPSD::Instance &problem,
                                                      FORPFSSPSD::MachineId machineId) {
    const auto &dg = problem.getDelayGraph();
    const auto &ops = problem.getMachineOperations(machineId);
    const auto &jobs = problem.jobs();

    std::unordered_set<FORPFSSPSD::OperationId> machineOpsSet(ops.begin(), ops.end());
    DelayGraph::Edges result;

    std::reference_wrapper<const DelayGraph::vertex> vPrevious = dg.get_source(machineId);
    for (const auto &[jobId, _] : jobs) {
        for (const auto &op : problem.getJobOperations(jobId)) {
            if (machineOpsSet.find(op.operationId) == machineOpsSet.end()) {
                continue;
            }

            const auto &vCurrent = dg.get_vertex(op);
            if (dg.has_edge(vPrevious, vCurrent)) {
                result.emplace_back(dg.get_edge(vPrevious, vCurrent));
            } else {
                auto value = problem.query(vPrevious, vCurrent);
                result.emplace_back(vPrevious.get().id, vCurrent.id, value);
            }
            vPrevious = vCurrent;
        }
    }

    return result;
}
