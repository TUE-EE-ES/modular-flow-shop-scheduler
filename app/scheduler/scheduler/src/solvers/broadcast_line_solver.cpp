#include "fms/pch/containers.hpp" // Precompiled headers always go first

#include "fms/solvers/broadcast_line_solver.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/delay.hpp"
#include "fms/math/interval.hpp"
#include "fms/problem/boundary.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/problem/module.hpp"
#include "fms/problem/operation.hpp"
#include "fms/scheduler.hpp"
#include "fms/solvers/modular_args.hpp"
#include "fms/solvers/partial_solution.hpp"
#include "fms/solvers/sequence.hpp"
#include "fms/solvers/solver.hpp"

#include <cstdint>
#include <ranges>

using namespace fms;
using namespace fms::solvers;

namespace {

template <bool negate = true>
std::optional<delay> getDifferenceOptional(const std::vector<delay> &ST,
                                           const cg::VertexId vPrevious,
                                           const cg::VertexId vNext) {
    if (ST[vNext] != algorithms::paths::kASAPStartValue) {
        if constexpr (negate) {
            return {ST[vPrevious] - ST[vNext]};
        }
        return {ST[vNext] - ST[vPrevious]};
    }
    return std::nullopt;
}

using VectorSideFunc = const problem::Operation &(const problem::OperationsVector &);

inline const problem::Operation &front(const problem::OperationsVector &v) { return v.front(); }

inline const problem::Operation &back(const problem::OperationsVector &v) { return v.back(); }

void updateBounds(std::unordered_map<problem::JobId, problem::TimeInterval> &intervals,
                  const problem::JobId jobId,
                  const std::optional<delay> &min,
                  const std::optional<delay> &max) {
    auto it = intervals.find(jobId);
    if (it != intervals.end()) {
        it->second.replace(min, max);
    } else {
        intervals.emplace(jobId, math::Interval<delay>{min, max});
    }
}

template <VectorSideFunc side>
void updateLowerBounds(problem::IntervalSpec &intervals,
                       const cg::VertexId vertexCurr,
                       std::size_t jobIndex,
                       const problem::Instance &problem,
                       const algorithms::paths::PathTimes &ASAPST) {
    const auto &jobsOut = problem.getJobsOutput();
    const auto jobIdCurr = jobsOut[jobIndex];
    auto &jobFstBounds = intervals[jobIdCurr];
    for (size_t jobIndexNext = jobIndex + 1; jobIndexNext < jobsOut.size(); ++jobIndexNext) {
        const auto jobIdNext = jobsOut[jobIndexNext];
        const auto &opNext = side(problem.jobs(jobIdNext));
        const auto vertexNext = problem.getDelayGraph().getVertex(opNext).id;

        const auto min = ASAPST[vertexNext] - ASAPST[vertexCurr];
        updateBounds(jobFstBounds, jobIdNext, min, {});
    }
}

template <VectorSideFunc side>
void updateUpperBounds(problem::IntervalSpec &intervals,
                       const cg::VertexId vertexCurr,
                       std::size_t jobIndex,
                       const problem::Instance &problem,
                       const algorithms::paths::PathTimes &ASAPSTStatic) {
    const auto &jobsOut = problem.getJobsOutput();
    const auto jobIdCurr = jobsOut[jobIndex];
    for (size_t jobIndexPrev = 0; jobIndexPrev < jobIndex; ++jobIndexPrev) {
        const auto jobIdPrev = jobsOut[jobIndexPrev];
        const auto &opPrev = side(problem.jobs(jobIdPrev));
        const auto vertexPrev = problem.getDelayGraph().getVertexId(opPrev);

        // For the upper bound, check the distance from the current job to the previous job
        std::optional max = getDifferenceOptional(ASAPSTStatic, vertexCurr, vertexPrev);
        updateBounds(intervals[jobIdPrev], jobIdCurr, {}, max);
    }
}

template <VectorSideFunc side>
void computeAndAddBounds(problem::IntervalSpec &bounds,
                         const problem::Module &problem,
                         cg::ConstraintGraph &dg,
                         const PartialSolution &solution,
                         const size_t jobIndex,
                         const bool upperBound = false) {
    const auto &jobsOut = problem.getJobsOutput();

    // Gets the first or last operation depending on Side()
    const auto &opCurr = side(problem.jobs(jobsOut[jobIndex]));
    const auto vertexCurr = dg.getVertexId(opCurr);
    const bool isNotLast = jobIndex + 1 < jobsOut.size();
    const bool isNotFirst = jobIndex > 0;

    if (isNotFirst && !upperBound) {
        // Upper bound is static only
        const auto ASAPSTStatic = algorithms::paths::computeASAPSTFromNode(dg, vertexCurr);
        updateUpperBounds<side>(bounds, vertexCurr, jobIndex, problem, ASAPSTStatic);
    }

    if (isNotLast || upperBound) {
        const auto edges = solution.getAllChosenEdges(problem);
        const auto ASAPST = algorithms::paths::computeASAPSTFromNode(dg, vertexCurr, edges);
        if (isNotLast) {
            updateLowerBounds<side>(bounds, vertexCurr, jobIndex, problem, ASAPST);
        }

        if (upperBound && isNotFirst) {
            updateUpperBounds<side>(bounds, vertexCurr, jobIndex, problem, ASAPST);
        }
    }
}

nlohmann::json saveAllSequences(const std::vector<ModulesSolutions> &solutions,
                                const problem::ProductionLine &problem) {
    nlohmann::json result;
    for (const auto &solution : solutions) {
        auto sequences = sequence::saveProductionLineSequences(solution, problem);
        result.push_back(sequences);
    }
    return result;
}

} // namespace

std::tuple<std::vector<ProductionLineSolution>, nlohmann::json>
BroadcastLineSolver::solve(problem::ProductionLine &problem, const cli::CLIArgs &args) {

    const auto argsMod = cli::ModularArgs::fromArgs(args);
    uint64_t iterations = 0;

    auto &modules = problem.modules();
    DistributedSchedulerHistory history(argsMod.storeSequence, argsMod.storeBounds);
    // Wait for convergence of lower bound before enabling upper bound
    bool convergedLowerBound = false;

    while (iterations < argsMod.maxIterations && argsMod.timer.isRunning()) {
        ModulesSolutions moduleResults;
        problem::GlobalBounds newIntervals;
        newIntervals.reserve(modules.size());
        const bool upperBound = convergedLowerBound;

        for (auto &[moduleId, m] : modules) {
            m.setIteration(iterations);
            try {
                auto [result, algorithmData] =
                        Scheduler::runAlgorithm(problem, m, args, iterations);
                history.addAlgorithmData(moduleId, std::move(algorithmData));

                auto bounds = getBounds(m, result.front(), upperBound);

                if (argsMod.selfBounds) {
                    m.addInputBounds(bounds.in);
                    m.addOutputBounds(bounds.out);
                }

                newIntervals.emplace(moduleId, std::move(bounds));
                moduleResults.emplace(moduleId, std::move(result.front()));
            } catch (FmsSchedulerException &e) {
                LOG_E("Broadcast: Exception while running algorithm: {}", e.what());
                auto result = baseResultData(history, problem, iterations);
                result["error"] = ErrorStrings::kLocalScheduler;
                return {ProductionLineSolutions{}, std::move(result)};
            }
        }

        history.addIteration(moduleResults, newIntervals);
        const auto [transIntervals, converged] = translateBounds(problem, newIntervals);
        propagateIntervals(problem, transIntervals);
        convergedLowerBound |= converged;

        ++iterations;

        if (converged && upperBound) {
            return {ProductionLineSolutions{mergeSolutions(problem, moduleResults)},
                    baseResultData(history, problem, iterations)};
        }
    }

    auto result = baseResultData(history, problem, iterations);
    result["error"] = ErrorStrings::kNoConvergence;
    if (argsMod.timer.isTimeUp()) {
        LOG_W("Broadcast: Time limit reached");
        result["timeout"] = true;
        result["error"] = ErrorStrings::kTimeOut;
    }
    return {ProductionLineSolutions{}, std::move(result)};
}

problem::ModuleBounds BroadcastLineSolver::getBounds(const problem::Module &problem,
                                                     const PartialSolution &solution,
                                                     const bool upperBound,
                                                     const BoundsSide intervalSide) {
    problem::ModuleBounds result;
    auto dg = problem.getDelayGraph(); // Copy delay graph because we are going to modify it
    // Find the bounds for each job
    for (size_t i = 0; i < problem.getJobsOutput().size(); ++i) {
        if (intervalSide == BoundsSide::INPUT || intervalSide == BoundsSide::BOTH) {
            computeAndAddBounds<front>(result.in, problem, dg, solution, i, upperBound);
        }

        if (intervalSide == BoundsSide::OUTPUT || intervalSide == BoundsSide::BOTH) {
            computeAndAddBounds<back>(result.out, problem, dg, solution, i, upperBound);
        }
    }

    return result;
}

std::tuple<problem::GlobalBounds, bool>
BroadcastLineSolver::translateBounds(const problem::ProductionLine &problem,
                                     const problem::GlobalBounds &intervals) {
    problem::GlobalBounds result;
    bool converged = true;
    for (const auto &[moduleId, modIntervals] : intervals) {
        const auto &m = problem[moduleId];
        if (problem.hasPrevModule(m)) {
            const auto &prevModule = problem.getPrevModule(m);
            const auto translated = problem.toOutputBounds(prevModule, modIntervals.in);
            result[prevModule.getModuleId()].out = translated;

            const auto &intervalsPrevious = intervals.at(prevModule.getModuleId());
            converged &= isConverged(translated, intervalsPrevious.out);
        }

        if (problem.hasNextModule(m)) {
            const auto &nextModule = problem.getNextModule(m);
            const auto translated = problem.toInputBounds(nextModule, modIntervals.out);
            result[nextModule.getModuleId()].in = translated;

            const auto &intervalsNext = intervals.at(nextModule.getModuleId());
            converged &= isConverged(translated, intervalsNext.in);
        }
    }

    return {std::move(result), converged};
}

void BroadcastLineSolver::propagateIntervals(problem::ProductionLine &problem,
                                             const problem::GlobalBounds &translatedIntervals) {
    for (const auto &[moduleId, moduleIntervals] : translatedIntervals) {
        auto &m = problem[moduleId];
        if (problem.hasPrevModule(m)) {
            m.addInputBounds(moduleIntervals.in);
        }

        if (problem.hasNextModule(m)) {
            m.addOutputBounds(moduleIntervals.out);
        }
    }
}

ProductionLineSolution BroadcastLineSolver::mergeSolutions(const problem::ProductionLine &problem,
                                                           ModulesSolutions &modulesSolutions) {
    ModulesSolutions result;
    const auto &modulesIds = problem.moduleIds();
    result.reserve(modulesIds.size());
    result.emplace(modulesIds.front(), modulesSolutions.at(modulesIds.front()));

    for (std::size_t i = 1; i < modulesIds.size(); ++i) {
        const auto &module = problem.getModule(modulesIds[i]);
        auto &solution = modulesSolutions.at(modulesIds[i]);
        auto ASAPST = solution.getASAPST();
        auto dg = module.getDelayGraph();

        // Get the timings of the previous module and apply the constraints that tie them to the
        // current module
        const auto &modPrev = problem.getPrevModule(module);
        const auto &ASAPSTPrev = modulesSolutions.at(modPrev.getModuleId()).getASAPST();
        const auto &dgPrev = modPrev.getDelayGraph();

        for (const auto &[jobId, ops] : modPrev.jobs()) {
            // Find the output time of the job
            const auto timeOutput = ASAPSTPrev.at(dgPrev.getVertexId(ops.back()));

            // Update start times of the first operation in the current module with the output time
            // of the same job on the previous module and the transfer constraints
            const auto &vertexId = dg.getVertexId(module.jobs(jobId).front());
            ASAPST[vertexId] = timeOutput + problem.query(modPrev, jobId);
        }

        // Check for positive cycles with the new ASAPST
        const auto edgesSequence = solution.getAllChosenEdges(module);
        const auto pathResult = algorithms::paths::computeASAPST(dg, ASAPST, edgesSequence);

        if (!pathResult.positiveCycle.empty()) {
            throw FmsSchedulerException(
                    "Modular merge: Adding start times caused a positive cycle!");
        }

        // Check that all jobs respect the due date
        for (const auto &[jobId, ops] : modPrev.jobs()) {
            const auto timePrev = ASAPSTPrev.at(dgPrev.getVertexId(ops.back()));

            const auto &vertexId = dg.getVertexId(module.jobs(jobId).front());
            const auto timeDiff = ASAPST.at(vertexId) - timePrev;

            const auto &dueDate = problem.getTransferDueDate(modPrev, jobId);
            if (dueDate && timeDiff > *dueDate) {
                throw FmsSchedulerException(
                        fmt::format(FMT_COMPILE("Job {} exceeds due date {}"), jobId, *dueDate));
            }
        }
        solution.setASAPST(ASAPST);
        result.emplace(module.getModuleId(),
                       PartialSolution{solution.getChosenSequencesPerMachine(), std::move(ASAPST)});
    }

    const auto &moduleLast = problem.getLastModule();

    return {result.at(moduleLast.getModuleId()).getRealMakespan(moduleLast), result};
}

bool BroadcastLineSolver::isConverged(const problem::IntervalSpec &sender,
                                      const problem::IntervalSpec &receiver) {
    if (sender.size() != receiver.size()) {
        return false;
    }

    for (const auto &[jobId, jobBoundsS] : sender) {
        const auto it = receiver.find(jobId);
        if (it == receiver.end()) {
            return false;
        }
        const auto &jobBoundsR = it->second;

        if (jobBoundsS.size() != jobBoundsR.size()) {
            return false;
        }

        for (const auto &[jobId2, boundS] : jobBoundsS) {
            const auto it2 = jobBoundsR.find(jobId2);
            if (it2 == jobBoundsR.end()) {
                return false;
            }

            if (!boundS.converged(it2->second)) {
                return false;
            }
        }
    }

    return true;
}

nlohmann::json BroadcastLineSolver::baseResultData(const DistributedSchedulerHistory &history,
                                                   const problem::ProductionLine &problem,
                                                   std::uint64_t iterations) {

    nlohmann::json result = history.toJSON(problem);
    return {{"productionLine", std::move(result)}, {"iterations", iterations}};
}
