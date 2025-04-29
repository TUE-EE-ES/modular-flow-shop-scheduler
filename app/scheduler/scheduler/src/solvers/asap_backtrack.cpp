#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"
#include "fms/pch/utils.hpp"

#include "fms/solvers/asap_backtrack.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/solvers/forward_heuristic.hpp"
#include "fms/solvers/utils.hpp"

using namespace fms;
using namespace fms::solvers;

namespace {
std::size_t findInsertionPoint(problem::Instance &problem,
                               const Sequence &sequence,
                               const problem::Operation &op,
                               std::size_t lastInsertionPoint) {

    const auto &jobs = problem.getJobsOutput();
    const auto &firstJob = jobs.front();

    for (std::size_t i = lastInsertionPoint; i < sequence.size(); ++i) {
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

PartialSolution AsapBacktrack::solve(problem::Instance &problem, const cli::CLIArgs &args) {
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

    const auto jobs = problem.getJobsOutput();
    std::vector<problem::Operation> toScheduleOps;

    for (std::size_t i = 0; i + 1 < jobs.size(); ++i) {
        const auto &jobOps = problem.getJobOperationsOnMachine(jobs[i], reEntrantMachine);

        // First operation is already included in the initial sequence
        for (std::size_t i = 1; i < jobOps.size(); ++i) {
            toScheduleOps.push_back(jobOps[i]);
        }
    }

    const std::size_t totalOps = toScheduleOps.size() + initialSequence.size();
    std::size_t currentOpIdx = 0;
    std::vector<std::size_t> lastInsertionPoints(toScheduleOps.size(), 0);

	auto timer = utils::time::StaticTimer(args.timeOut * jobs.size());

    while (currentOpIdx < toScheduleOps.size() && timer.isRunning()) {
        const auto &op = toScheduleOps[currentOpIdx];

        const auto insertionPoint = scheduleOneOperation(
                dg, problem, op, initialSequence, lastInsertionPoints[currentOpIdx], ASAPST);

        if (insertionPoint.has_value()) {
            lastInsertionPoints[currentOpIdx] = insertionPoint.value();
            ++currentOpIdx;

            if (currentOpIdx < toScheduleOps.size()) {
                lastInsertionPoints[currentOpIdx] = insertionPoint.value() + 1;
            }
        } else {
            if (currentOpIdx == 0) {
                throw FmsSchedulerException("No solution found");
            }

            // Backtrack: remove last inserted operation
            --currentOpIdx;
            initialSequence.erase(initialSequence.begin() + lastInsertionPoints[currentOpIdx]);
            lastInsertionPoints[currentOpIdx]++;
        }
    }

    auto testEdges = SolversUtils::getAllEdgesPlusInferredEdges(problem, initialSequence);
    auto result = algorithms::paths::computeASAPST(dg, testEdges);
    if (result.hasPositiveCycle() || initialSequence.size() != totalOps) {
        throw FmsSchedulerException("Infeasible solution found");
    }

    PartialSolution solution({{reEntrantMachine, std::move(initialSequence)}}, {});
    return solution;
}

std::optional<std::size_t>
fms::solvers::AsapBacktrack::scheduleOneOperation(cg::ConstraintGraph &dg,
                                                  problem::Instance &problem,
                                                  const problem::Operation &eligibleOperation,
                                                  Sequence &currentSequence,
                                                  std::size_t lastInsertionPoint,
                                                  algorithms::paths::PathTimes &ASAPST) {
    const auto insertionPoint =
            findInsertionPoint(problem, currentSequence, eligibleOperation, lastInsertionPoint);

    for (std::size_t i = insertionPoint; i < currentSequence.size(); ++i) {
        currentSequence.insert(currentSequence.begin() + i, eligibleOperation);
        auto testEdges = SolversUtils::getAllEdgesPlusInferredEdges(problem, currentSequence);

        algorithms::paths::initializeASAPST(dg, ASAPST);
        auto result = algorithms::paths::computeASAPST(dg, ASAPST, testEdges);
        if (!result.hasPositiveCycle()) {
            return i;
        } else {
            currentSequence.erase(currentSequence.begin() + i);
        }
    }

    return std::nullopt;
}
