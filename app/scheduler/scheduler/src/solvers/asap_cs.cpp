#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/solvers/asap_cs.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/builder.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/edge.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/problem/operation.hpp"
#include "fms/solvers/forward_heuristic.hpp"
#include "fms/solvers/maintenance_heuristic.hpp"
#include "fms/solvers/utils.hpp"

using namespace fms;
using namespace fms::solvers;

namespace {
std::size_t findInsertionPoint(problem::Instance &problem,
                               const Sequence &sequence,
                               const problem::Operation &op,
                               std::optional<std::size_t> lastInsertionPoint) {

    const auto &jobs = problem.getJobsOutput();
    const auto &firstJob = jobs.front();

    for (std::size_t i = lastInsertionPoint.value_or(0); i < sequence.size(); ++i) {
        const auto &currOp = sequence[i];
        if (currOp.jobId == firstJob && op.jobId == firstJob) {
            return i + 1;
        }

        if (currOp.jobId > op.jobId) {
            return i;
        }
    }
    return sequence.size();
}
} // namespace

PartialSolution ASAPCS::solve(problem::Instance &problem, const cli::CLIArgs &args) {
    // solve the instance
    LOG("Computation of the schedule started");

    auto ASAPST = SolversUtils::initProblemGraph(problem, IS_LOG_D());
    auto dg = problem.getDelayGraph();

    // We only support a single re-entrant machine in the system so choose the first one
    problem::MachineId reEntrantMachine = problem.getReEntrantMachines().front();
    if (problem.getMachineOperations(reEntrantMachine).size() > 2) {
        throw std::runtime_error("Multiple re-entrancies not implemented yet");
    }

    Sequence initialSequence = forward::createInitialSequence(problem, reEntrantMachine);
    PartialSolution solution({{reEntrantMachine, std::move(initialSequence)}}, std::move(ASAPST));

    const auto jobs = problem.getJobsOutput();
    std::optional<std::size_t> lastInsertionPoint;

    for (std::size_t i = 0; i + 1 < jobs.size(); ++i) {
        const auto &jobOps = problem.getJobOperationsOnMachine(jobs[i], reEntrantMachine);

        // First operation is already included in the initial sequence
        for (std::size_t i = 1; i < jobOps.size(); ++i) {
            std::tie(solution, lastInsertionPoint) = scheduleOneOperation(
                    dg, problem, reEntrantMachine, solution, jobOps.at(i), lastInsertionPoint);
        }
    }

    return solution;
}

std::tuple<PartialSolution, std::size_t>
fms::solvers::ASAPCS::scheduleOneOperation(cg::ConstraintGraph &dg,
                                           problem::Instance &problem,
                                           problem::MachineId reEntrantMachine,
                                           const PartialSolution &currentSolution,
                                           const problem::Operation &eligibleOperation,
                                           std::optional<std::size_t> lastInsertionPoint) {
    const auto currentSequence =
            currentSolution.getMachineSequence(problem.getReEntrantMachines().front());

    const auto insertionPoint =
            findInsertionPoint(problem, currentSequence, eligibleOperation, lastInsertionPoint);

    for (std::size_t i = insertionPoint; i < currentSequence.size(); ++i) {
        // Copy the current sequence up to the insertion point, then insert the operation and then
        // insert the rest of the current sequence
        Sequence newSequence;
        newSequence.reserve(currentSequence.size() + 1);
        newSequence.insert(newSequence.end(), currentSequence.begin(), currentSequence.begin() + i);
        newSequence.push_back(eligibleOperation);
        newSequence.insert(newSequence.end(), currentSequence.begin() + i, currentSequence.end());

        PartialSolution solution({{reEntrantMachine, std::move(newSequence)}}, {});
        auto finalSequence = solution.getAllAndInferredEdges(problem);

        auto result = algorithms::paths::computeASAPST(dg, finalSequence);
        if (!result.hasPositiveCycle()) {
            solution.setASAPST(std::move(result.times));
            return {std::move(solution), i};
        }
    }

    throw FmsSchedulerException(
            fmt::format("No feasible option has been detected for operation {}. This is not "
                        "possible in the Canon case",
                        eligibleOperation));
}
