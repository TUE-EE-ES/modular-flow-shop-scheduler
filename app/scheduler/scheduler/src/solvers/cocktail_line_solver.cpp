#include "pch/containers.hpp" // Precompiled headers always go first

#include "solvers/cocktail_line_solver.hpp"

#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/module.hpp"
#include "delayGraph/builder.hpp"
#include "fmsscheduler.h"
#include "solvers/broadcast_line_solver.hpp"
#include "solvers/modular_args.hpp"
#include "solvers/solver.h"

#include <fmt/compile.h>

using namespace algorithm;
using namespace algorithm::BroadcastLineSolver;
namespace FS = FORPFSSPSD;

namespace {

void historyAddEmpty(std::vector<FMS::ModulesSolutions> &allResults,
                     std::vector<GlobalIntervals> &allBounds,
                     const ModularArgs &argsMod) {
    if (argsMod.storeSequence) {
        allResults.emplace_back();
    }

    if (argsMod.storeBounds) {
        allBounds.emplace_back();
    }
}

std::tuple<std::vector<FMS::ModulesSolutions>, std::vector<GlobalIntervals>>
baseHistory(const ModularArgs &argsMod) {
    std::vector<FMS::ModulesSolutions> allResults;
    std::vector<GlobalIntervals> allBounds;
    historyAddEmpty(allResults, allBounds, argsMod);
    return {std::move(allResults), std::move(allBounds)};
}

void historyAddModule(std::vector<FMS::ModulesSolutions> &allResults,
                      std::vector<GlobalIntervals> &allBounds,
                      const ModularArgs &argsMod,
                      const FS::ModuleId moduleId,
                      const ModuleBounds &bounds,
                      const PartialSolution &modResult) {
    if (argsMod.storeSequence) {
        allResults.back().emplace(moduleId, modResult);
    }

    if (argsMod.storeBounds) {
        allBounds.back().emplace(moduleId, bounds);
    }
}

} // namespace

CocktailLineSolver::SingleIterationResult
CocktailLineSolver::singleIteration(FS::ProductionLine &instance,
                                    const commandLineArgs &args,
                                    const uint64_t iterations,
                                    const bool convergedLowerBound,
                                    const ModularArgs &argsMod) {
    FS::ModuleId moduleId = instance.getFirstModuleId();
    ModuleBounds bounds;
    FMS::ModulesSolutions moduleResults;
    auto [allResults, allBounds] = baseHistory(argsMod);

    bool first = true;
    bool canContinue = true;
    const bool upperBound = convergedLowerBound;

    // Iterations forward
    while (canContinue && argsMod.timer.isRunning()) {
        auto &module = instance[moduleId];
        const auto currentModuleId = moduleId;
        module.setIteration(fmt::format("{}F", iterations));
        canContinue = instance.hasNextModule(moduleId);

        if (!first) {
            // Add the input intervals from the previous iteration
            module.addInputBounds(instance.toInputBounds(module, bounds.out));
        }
        first = false;

        auto side = BoundsSide::INPUT;
        if (canContinue) {
            side = BoundsSide::OUTPUT;
            moduleId = instance.getNextModuleId(module);
        }

        try {
            auto [result, _] = FmsScheduler::runAlgorithm(module, args, 2 * iterations);
            auto &modResult = result.front();

            bounds = getBounds(module, modResult, upperBound, side);

            if (argsMod.selfBounds) {
                module.addInputBounds(bounds.in);
                module.addOutputBounds(bounds.out);
            }

            historyAddModule(allResults, allBounds, argsMod, currentModuleId, bounds, modResult);

            if (!canContinue) {
                moduleResults.emplace(moduleId, std::move(modResult));
            }
        } catch (FmsSchedulerException &e) {
            LOG_E("Cocktail: Exception while running algorithm: {}", e.what());
            return {{},
                    false,
                    ErrorStrings::kLocalScheduler,
                    std::move(allResults),
                    std::move(allBounds)};
        }
    }

    historyAddEmpty(allResults, allBounds, argsMod);

    first = true;
    canContinue = true;
    bool converged = true;

    // Iterations backwards
    while (canContinue && argsMod.timer.isRunning()) {
        auto &module = instance[moduleId];
        const auto currentModuleId = moduleId;
        module.setIteration(fmt::format(FMT_COMPILE("{}B"), iterations));

        // Update the module ID
        canContinue = instance.hasPrevModule(moduleId);
        if (canContinue) {
            moduleId = instance.getPrevModuleId(module);
        }

        if (first) {
            // The first iteration backwards is the last one forwards
            first = false;
            continue;
        }

        // Add output intervals from previous iteration
        auto translated = instance.toOutputBounds(module, bounds.in);
        module.addOutputBounds(translated);

        try {
            auto [result, _] = FmsScheduler::runAlgorithm(module, args, 2 * iterations + 1);
            auto &modResult = result.front();

            // We use both sides because one side is used for propagation and the other for
            // convergence check
            bounds = getBounds(module, modResult, upperBound, BoundsSide::BOTH);

            if (argsMod.selfBounds) {
                module.addInputBounds(bounds.in);
                module.addOutputBounds(bounds.out);
            }

            historyAddModule(allResults, allBounds, argsMod, currentModuleId, bounds, modResult);

            // If the result of applying the intervals and rescheduling still provides the same
            // bounds then this means that convergence has been reached.
            converged &= isConverged(translated, bounds.out);
            moduleResults.emplace(currentModuleId, std::move(modResult));
        } catch (FmsSchedulerException &e) {
            LOG_E("Cocktail: Exception while running algorithm: {}", e.what());
            return {{},
                    false,
                    ErrorStrings::kLocalScheduler,
                    std::move(allResults),
                    std::move(allBounds)};
        }
    }

    if (argsMod.timer.isTimeUp()) {
        LOG_W("Cocktail: Time limit reached");
        return {{}, false, ErrorStrings::kTimeOut, std::move(allResults), std::move(allBounds)};
    }

    return {std::move(moduleResults), converged, "", std::move(allResults), std::move(allBounds)};
}

std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
CocktailLineSolver::solve(FS::ProductionLine &problemInstance, const commandLineArgs &args) {
    uint64_t iterations = 0;
    const auto argsMod = ModularArgs::fromArgs(args);

    auto &modules = problemInstance.modules();
    bool convergedLowerBound = false;
    std::vector<GlobalIntervals> allBounds;                // To store the bounds history
    std::vector<FMS::ModulesSolutions> allSolutions;       // To store the solutions history

    // Generate the delay graphs of each module
    for (auto &[moduleId, module] : modules) {
        module.updateDelayGraph(DelayGraph::Builder::build(module));
    }

    FS::ModuleId moduleId = problemInstance.getFirstModuleId();
    std::string globalErrorStr;

    while (iterations < args.maxIterations && argsMod.timer.isRunning() && globalErrorStr.empty()) {
        auto [moduleResults, converged, errorStr, iterationResults, iterationBounds] =
                singleIteration(problemInstance, args, iterations, convergedLowerBound, argsMod);

        allBounds.insert(allBounds.end(), iterationBounds.begin(), iterationBounds.end());
        allSolutions.insert(allSolutions.end(), iterationResults.begin(), iterationResults.end());

        if (!errorStr.empty()) {
            globalErrorStr = std::move(errorStr);
            break;
        }

        if (converged && convergedLowerBound) {
            return {{mergeSolutions(problemInstance, moduleResults)},
                    baseResultData(allSolutions, allBounds, problemInstance, iterations)};
        }

        convergedLowerBound |= converged;
        ++iterations;
    }

    auto data = baseResultData(allSolutions, allBounds, problemInstance, iterations);
    data["timeout"] = argsMod.timer.isTimeUp();
    if (!globalErrorStr.empty()) {
        data["error"] = globalErrorStr;
    } else {
        data["error"] = ErrorStrings::kNoConvergence;
    }

    return {ProductionLineSolutions{}, std::move(data)};
}
