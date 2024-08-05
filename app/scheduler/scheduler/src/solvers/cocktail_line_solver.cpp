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

CocktailLineSolver::SingleIterationResult
CocktailLineSolver::singleIteration(FS::ProductionLine &instance,
                                    const commandLineArgs &args,
                                    const uint64_t iterations,
                                    const bool convergedLowerBound,
                                    const ModularArgs &argsMod,
                                    FMS::DistributedSchedulerHistory &history) {
    FS::ModuleId moduleId = instance.getFirstModuleId();
    FS::ModuleBounds bounds;
    FMS::ModulesSolutions moduleResults;
    history.newIteration();

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

            history.addModule(currentModuleId, bounds, modResult);
            if (!canContinue) {
                moduleResults.emplace(moduleId, std::move(modResult));
            }
        } catch (FmsSchedulerException &e) {
            LOG_E("Cocktail: Exception while running algorithm: {}", e.what());
            return {{}, false, ErrorStrings::kLocalScheduler};
        }
    }

    history.newIteration();
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
        auto oldBoundsIn = std::move(bounds.in);
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

            history.addModule(currentModuleId, bounds, modResult);


            // While we are iterating backwards, (e.g., n <- n_{i-1}), we are sending the bounds
            // forward (e.g., n_{i} sends to n_{i+1}). We can detect convergence by checking that 
            // the bounds that we are sending forwards will not cause module n_{i+1} to update
            // its local problem. It won't cause an update if the bounds that we are sending
            // are "weaker" than the bounds that module n_{i+1} already sent to module n_{i}.
            // If this is true for all iterations backwards, then we can stop the iterations.
            const auto &moduleNext = instance.getNextModule(module);
            auto translatedBack = instance.toInputBounds(moduleNext, bounds.out);
            converged &= isConverged(translatedBack, oldBoundsIn);

            moduleResults.emplace(currentModuleId, std::move(modResult));
        } catch (FmsSchedulerException &e) {
            LOG_E("Cocktail: Exception while running algorithm: {}", e.what());
            return {{}, false, ErrorStrings::kLocalScheduler};
        }
    }

    if (argsMod.timer.isTimeUp()) {
        LOG_W("Cocktail: Time limit reached");
        return {{}, false, ErrorStrings::kTimeOut};
    }

    return {std::move(moduleResults), converged, ""};
}

std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
CocktailLineSolver::solve(FS::ProductionLine &problemInstance, const commandLineArgs &args) {
    uint64_t iterations = 0;
    const auto argsMod = ModularArgs::fromArgs(args);

    auto &modules = problemInstance.modules();
    bool convergedLowerBound = false;

    // Generate the delay graphs of each module
    for (auto &[moduleId, module] : modules) {
        module.updateDelayGraph(DelayGraph::Builder::build(module));
    }

    FS::ModuleId moduleId = problemInstance.getFirstModuleId();
    std::string globalErrorStr;
    FMS::DistributedSchedulerHistory history(argsMod.storeSequence, argsMod.storeBounds);

    while (iterations < args.maxIterations && argsMod.timer.isRunning() && globalErrorStr.empty()) {
        auto [moduleResults, converged, errorStr] =
                singleIteration(problemInstance, args, iterations, convergedLowerBound, argsMod, history);

        if (!errorStr.empty()) {
            globalErrorStr = std::move(errorStr);
            break;
        }

        ++iterations;

        if (converged && convergedLowerBound) {
            return {{mergeSolutions(problemInstance, moduleResults)},
                    baseResultData(history, problemInstance, iterations)};
        }

        convergedLowerBound |= converged;
    }

    auto data = baseResultData(history, problemInstance, iterations);
    data["timeout"] = argsMod.timer.isTimeUp();
    if (!globalErrorStr.empty()) {
        data["error"] = globalErrorStr;
    } else {
        data["error"] = ErrorStrings::kNoConvergence;
    }

    return {ProductionLineSolutions{}, std::move(data)};
}
