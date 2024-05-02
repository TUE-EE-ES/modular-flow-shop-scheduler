#include "pch/containers.hpp" // Precompiled headers always go first

#include "solvers/broadcast_line_solver.hpp"

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/boundary.hpp"
#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/module.hpp"
#include "FORPFSSPSD/operation.h"
#include "delay.h"
#include "delayGraph/delayGraph.h"
#include "fmsscheduler.h"
#include "longest_path.h"
#include "math/interval.hpp"
#include "partialsolution.h"
#include "solvers/modular_args.hpp"
#include "solvers/sequence.hpp"
#include "solvers/solver.h"

#include <cstdint>
#include <ranges>

using namespace algorithm;
using namespace DelayGraph;

namespace FS = FORPFSSPSD;

namespace {

template <bool negate = true>
std::optional<delay> getDifferenceOptional(const std::vector<delay> &ST,
                                           const VertexID vPrevious,
                                           const VertexID vNext) {
    if (ST[vNext] != LongestPath::kASAPStartValue) {
        if constexpr (negate) {
            return {ST[vPrevious] - ST[vNext]};
        }
        return {ST[vNext] - ST[vPrevious]};
    }
    return std::nullopt;
}

using VectorSideFunc = const FS::operation &(const FS::OperationsVector &);

inline const FS::operation &front(const FS::OperationsVector &v) { return v.front(); }

inline const FS::operation &back(const FS::OperationsVector &v) { return v.back(); }

void updateBounds(std::unordered_map<FS::JobId, FS::TimeInterval> &intervals,
                  const FS::JobId jobId,
                  const std::optional<delay> &min,
                  const std::optional<delay> &max) {
    auto it = intervals.find(jobId);
    if (it != intervals.end()) {
        it->second.replace(min, max);
    } else {
        intervals.emplace(jobId, Math::Interval<delay>{min, max});
    }
}

template <VectorSideFunc side>
void updateLowerBounds(FORPFSSPSD::IntervalSpec &intervals,
                       const VertexID vertexCurr,
                       std::size_t jobIndex,
                       const FS::Instance &problem,
                       const PathTimes &ASAPST) {
    const auto &jobsOut = problem.getJobsOutput();
    const auto jobIdCurr = jobsOut[jobIndex];
    auto &jobFstBounds = intervals[jobIdCurr];
    for (size_t jobIndexNext = jobIndex + 1; jobIndexNext < jobsOut.size(); ++jobIndexNext) {
        const auto jobIdNext = jobsOut[jobIndexNext];
        const auto &opNext = side(problem.getJobOperations(jobIdNext));
        const auto vertexNext = problem.getDelayGraph().get_vertex(opNext).id;

        const auto min = ASAPST[vertexNext] - ASAPST[vertexCurr];
        updateBounds(jobFstBounds, jobIdNext, min, {});
    }
}

template <VectorSideFunc side>
void updateUpperBounds(FORPFSSPSD::IntervalSpec &intervals,
                       const VertexID vertexCurr,
                       std::size_t jobIndex,
                       const FS::Instance &problem,
                       const PathTimes &ASAPSTStatic) {
    const auto &jobsOut = problem.getJobsOutput();
    const auto jobIdCurr = jobsOut[jobIndex];
    for (size_t jobIndexPrev = 0; jobIndexPrev < jobIndex; ++jobIndexPrev) {
        const auto jobIdPrev = jobsOut[jobIndexPrev];
        const auto &opPrev = side(problem.getJobOperations(jobIdPrev));
        const auto vertexPrev = problem.getDelayGraph().get_vertex_id(opPrev);

        // For the upper bound, check the distance from the current job to the previous job
        std::optional max = getDifferenceOptional(ASAPSTStatic, vertexCurr, vertexPrev);
        updateBounds(intervals[jobIdPrev], jobIdCurr, {}, max);
    }
}

template <VectorSideFunc side>
void computeAndAddBounds(FORPFSSPSD::IntervalSpec &bounds,
                         const FORPFSSPSD::Module &problem,
                         delayGraph &dg,
                         const PartialSolution &solution,
                         const size_t jobIndex,
                         const bool upperBound = false) {
    const auto &jobsOut = problem.getJobsOutput();

    // Gets the first or last operation depending on Side()
    const auto &opCurr = side(problem.getJobOperations(jobsOut[jobIndex]));
    const auto vertexCurr = dg.get_vertex(opCurr).id;
    const bool isNotLast = jobIndex + 1 < jobsOut.size();
    const bool isNotFirst = jobIndex > 0;

    if (isNotFirst && !upperBound) {
        // Upper bound is static only
        const auto ASAPSTStatic = LongestPath::computeASAPSTFromNode(dg, vertexCurr);
        updateUpperBounds<side>(bounds, vertexCurr, jobIndex, problem, ASAPSTStatic);
    }

    if (isNotLast || upperBound) {
        const auto edges = solution.getAllChosenEdges();
        const auto ASAPST = LongestPath::computeASAPSTFromNode(dg, vertexCurr, edges);
        if (isNotLast) {
            updateLowerBounds<side>(bounds, vertexCurr, jobIndex, problem, ASAPST);
        }

        if (upperBound && isNotFirst) {
            updateUpperBounds<side>(bounds, vertexCurr, jobIndex, problem, ASAPST);
        }
    }
}

nlohmann::json saveAllSequences(const std::vector<FMS::ModulesSolutions> &solutions,
                                const FORPFSSPSD::ProductionLine &problem) {
    nlohmann::json result;
    for (const auto &solution : solutions) {
        auto sequences = Sequence::saveProductionLineSequences(solution, problem);
        result.push_back(sequences);
    }
    return result;
}

} // namespace

nlohmann::json BroadcastLineSolver::boundsToJSON(const FORPFSSPSD::IntervalSpec &bounds) {
    nlohmann::json result;

    for (const auto &[jobFrom, mapTo] : bounds) {
        nlohmann::json jobIntervals;
        for (const auto &[jobTo, interval] : mapTo) {
            nlohmann::json value{interval.min().value_or(-1), interval.max().value_or(-1)};
            jobIntervals[fmt::to_string(jobTo)] = std::move(value);
        }
        result[fmt::to_string(jobFrom)] = jobIntervals;
    }
    return result;
}

nlohmann::json BroadcastLineSolver::boundsToJSON(const GlobalIntervals &globalBounds) {
    nlohmann::json result;
    for (const auto &[moduleId, moduleIntervals] : globalBounds) {
        const auto moduleIdInt = FORPFSSPSD::ModuleId(moduleId);
        nlohmann::json value{{"in", boundsToJSON(moduleIntervals.in)},
                             {"out", boundsToJSON(moduleIntervals.out)}};
        result[fmt::to_string(moduleIdInt)] = std::move(value);
    }
    return result;
}

nlohmann::json BroadcastLineSolver::boundsToJSON(const std::vector<GlobalIntervals> &bounds) {
    nlohmann::json result;
    for (const auto &globalIntervals : bounds) {
        result.push_back(boundsToJSON(globalIntervals));
    }
    return result;
}

FORPFSSPSD::IntervalSpec BroadcastLineSolver::moduleBoundsFromJSON(const nlohmann::json &json) {
    FS::IntervalSpec result;

    for (const auto &[jobFrom, mapTo] : json.items()) {
        FS::JobId jobFromId = std::stoi(jobFrom);
        for (const auto &[jobTo, intervalJson] : mapTo.items()) {
            FS::JobId jobToId = std::stoi(jobTo);

            if (intervalJson.size() != 2) {
                throw std::invalid_argument(fmt::format(
                        FMT_COMPILE("Invalid interval size for job {} to {}"), jobFromId, jobToId));
            }

            std::optional<delay> valueMin = intervalJson.at(0).get<delay>();
            std::optional<delay> valueMax = intervalJson.at(1).get<delay>();

            if (valueMin < 0) {
                valueMin = std::nullopt;
            }

            if (valueMax < 0) {
                valueMax = std::nullopt;
            }

            FS::TimeInterval interval{valueMin, valueMax};
            result[jobFromId].insert({jobToId, interval});
        }
    }
    return result;
}

GlobalIntervals BroadcastLineSolver::globalBoundsFromJSON(const nlohmann::json &json) {
    GlobalIntervals result;
    for (const auto &[moduleId, moduleIntervals] : json.items()) {
        auto moduleIdInt = FS::ModuleId(std::stoi(moduleId));
        result[moduleIdInt].in = moduleBoundsFromJSON(moduleIntervals.at("in"));
        result[moduleIdInt].out = moduleBoundsFromJSON(moduleIntervals.at("out"));
    }
    return result;
}

std::vector<GlobalIntervals>
BroadcastLineSolver::allGlobalBoundsFromJSON(const nlohmann::json &json) {
    if (!json.is_array()) {
        throw std::invalid_argument("Expected array of global intervals");
    }

    std::vector<GlobalIntervals> result;
    for (const auto &globalIntervals : json) {
        result.emplace_back(globalBoundsFromJSON(globalIntervals));
    }
    return result;
}

std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
BroadcastLineSolver::solve(FORPFSSPSD::ProductionLine &problem, const commandLineArgs &args) {

    const auto argsMod = ModularArgs::fromArgs(args);
    uint64_t iterations = 0;

    auto &modules = problem.modules();
    std::vector<GlobalIntervals> allIntervals;             // To store the bounds history
    std::vector<FMS::ModulesSolutions> allSolutions; // To store the solutions history
    // Wait for convergence of lower bound before enabling upper bound
    bool convergedLowerBound = false;

    while (iterations < args.maxIterations && argsMod.timer.isRunning()) {
        FMS::ModulesSolutions moduleResults;
        GlobalIntervals newIntervals;
        newIntervals.reserve(modules.size());
        const bool upperBound = convergedLowerBound;

        for (auto &[moduleId, m] : modules) {
            m.setIteration(iterations);
            try {
                auto [result, _] = FmsScheduler::runAlgorithm(m, args, iterations);
                auto bounds = getBounds(m, result.front(), upperBound);

                if (argsMod.selfBounds) {
                    m.addInputBounds(bounds.in);
                    m.addOutputBounds(bounds.out);
                }

                newIntervals.emplace(moduleId, std::move(bounds));
                moduleResults.emplace(moduleId, std::move(result.front()));
            } catch (FmsSchedulerException &e) {
                LOG_E("Broadcast: Exception while running algorithm: {}", e.what());
                auto result = baseResultData(allSolutions, allIntervals, problem, iterations);
                result["error"] = ErrorStrings::kLocalScheduler;
                return {ProductionLineSolutions{}, std::move(result)};
            }
        }

        if (argsMod.storeBounds) {
            allIntervals.push_back(newIntervals);
        }

        if (argsMod.storeSequence) {
            allSolutions.emplace_back(moduleResults);
        }

        const auto [transIntervals, converged] = translateBounds(problem, newIntervals);
        propagateIntervals(problem, transIntervals);
        convergedLowerBound |= converged;

        if (converged && upperBound) {
            return {ProductionLineSolutions{mergeSolutions(problem, moduleResults)},
                    baseResultData(allSolutions, allIntervals, problem, iterations)};
        }

        ++iterations;
    }

    auto result = baseResultData(allSolutions, allIntervals, problem, iterations);
    result["error"] = ErrorStrings::kNoConvergence;
    if (argsMod.timer.isTimeUp()) {
        LOG_W("Broadcast: Time limit reached");
        result["timeout"] = true;
        result["error"] = ErrorStrings::kTimeOut;
    }
    return {ProductionLineSolutions{}, std::move(result)};
}

ModuleBounds BroadcastLineSolver::getBounds(const FORPFSSPSD::Module &problem,
                                            const PartialSolution &solution,
                                            const bool upperBound,
                                            const BoundsSide intervalSide) {
    ModuleBounds result;
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

std::tuple<GlobalIntervals, bool>
algorithm::BroadcastLineSolver::translateBounds(const FORPFSSPSD::ProductionLine &problem,
                                                const GlobalIntervals &intervals) {
    GlobalIntervals result;
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

void BroadcastLineSolver::propagateIntervals(FORPFSSPSD::ProductionLine &problem,
                                             const GlobalIntervals &translatedIntervals) {
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

FMS::ProductionLineSolution
algorithm::BroadcastLineSolver::mergeSolutions(const FORPFSSPSD::ProductionLine &problem,
                                               FMS::ModulesSolutions &modulesSolutions) {
    FMS::ModulesSolutions result;
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
            const auto timeOutput = ASAPSTPrev.at(dgPrev.get_vertex_id(ops.back()));

            // Update start times of the first operation in the current module with the output time
            // of the same job on the previous module and the transfer constraints
            const auto &vertexId = dg.get_vertex_id(module.jobs(jobId).front());
            ASAPST[vertexId] = timeOutput + problem.query(modPrev, jobId);
        }

        // Check for positive cycles with the new ASAPST
        const auto edgesSequence = solution.getAllChosenEdges();
        const auto pathResult = LongestPath::computeASAPST(dg, ASAPST, edgesSequence);

        if (!pathResult.positiveCycle.empty()) {
            throw FmsSchedulerException(
                    "Modular merge: Adding start times caused a positive cycle!");
        }

        // Check that all jobs respect the due date
        for (const auto &[jobId, ops] : modPrev.jobs()) {
            const auto timePrev = ASAPSTPrev.at(dgPrev.get_vertex_id(ops.back()));

            const auto &vertexId = dg.get_vertex_id(module.jobs(jobId).front());
            const auto timeDiff = ASAPST.at(vertexId) - timePrev;

            const auto &dueDate = problem.getTransferDueDate(modPrev, jobId);
            if (dueDate && timeDiff > *dueDate) {
                throw FmsSchedulerException(
                        fmt::format(FMT_COMPILE("Job {} exceeds due date {}"), jobId, *dueDate));
            }
        }
        solution.setASAPST(ASAPST);
        result.emplace(module.getModuleId(),
                       PartialSolution{solution.getChosenEdgesPerMachine(), std::move(ASAPST)});
    }

    return {result.at(modulesIds.back()).getMakespan(), result};
}
bool algorithm::BroadcastLineSolver::isConverged(const FORPFSSPSD::IntervalSpec &boundsLeft,
                                                 const FORPFSSPSD::IntervalSpec &boundsRight) {
    if (boundsLeft.size() != boundsRight.size()) {
        return false;
    }

    for (const auto &[jobId, jobBoundsL] : boundsLeft) {
        const auto it = boundsRight.find(jobId);
        if (it == boundsRight.end()) {
            return false;
        }
        const auto &jobBoundsR = it->second;

        if (jobBoundsL.size() != jobBoundsR.size()) {
            return false;
        }

        for (const auto &[jobId2, boundL] : jobBoundsL) {
            const auto it2 = jobBoundsR.find(jobId2);
            if (it2 == jobBoundsR.end()) {
                return false;
            }

            if (!boundL.converged(it2->second)) {
                return false;
            }
        }
    }

    return true;
}

nlohmann::json algorithm::BroadcastLineSolver::baseResultData(
        const std::vector<FMS::ModulesSolutions> &solutions,
        const std::vector<GlobalIntervals> &bounds,
        const FORPFSSPSD::ProductionLine &problem,
        std::uint64_t iterations) {

    nlohmann::json result;
    if (!bounds.empty()) {
        result["bounds"] = BroadcastLineSolver::boundsToJSON(bounds);
    }
    if (!solutions.empty()) {
        result["sequences"] = saveAllSequences(solutions, problem);
    }

    return {{"productionLine", std::move(result)}, {"iterations", iterations}};
}
