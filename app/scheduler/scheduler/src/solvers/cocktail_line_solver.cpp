#include "fms/pch/containers.hpp" // Precompiled headers always go first
#include "fms/pch/fmt.hpp"

#include "fms/solvers/cocktail_line_solver.hpp"

#include "fms/cg/builder.hpp"
#include "fms/problem/indices.hpp"
#include "fms/problem/module.hpp"
#include "fms/scheduler.hpp"
#include "fms/solvers/broadcast_line_solver.hpp"
#include "fms/solvers/modular_args.hpp"
#include "fms/solvers/solver.hpp"

namespace fms::solvers::CocktailLineSolver {

SingleIterationResult singleIteration(problem::ProductionLine &instance,
                                      const cli::CLIArgs &args,
                                      const uint64_t iterations,
                                      const bool convergedLowerBound,
                                      const cli::ModularArgs &argsMod,
                                      DistributedSchedulerHistory &history) {
    problem::ModuleId moduleId = instance.getFirstModuleId();
    problem::ModuleBounds bounds;
    ModulesSolutions moduleResults;
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
            auto [result, algorithmData] =
                    Scheduler::runAlgorithm(instance, module, args, 2 * iterations);
            history.addAlgorithmData(moduleId, std::move(algorithmData));

            auto &modResult = result.front();

            bounds = BroadcastLineSolver::getBounds(module, modResult, upperBound, side);

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
            return {{}, false, BroadcastLineSolver::ErrorStrings::kLocalScheduler};
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
            auto [result, algorithmData] =
                    Scheduler::runAlgorithm(instance, module, args, 2 * iterations + 1);
            history.addAlgorithmData(moduleId, std::move(algorithmData));
            auto &modResult = result.front();

            // We use both sides because one side is used for propagation and the other for
            // convergence check
            bounds =
                    BroadcastLineSolver::getBounds(module, modResult, upperBound, BoundsSide::BOTH);

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
            converged &= BroadcastLineSolver::isConverged(translatedBack, oldBoundsIn);

            moduleResults.emplace(currentModuleId, std::move(modResult));
        } catch (FmsSchedulerException &e) {
            LOG_E("Cocktail: Exception while running algorithm: {}", e.what());
            return {{}, false, BroadcastLineSolver::ErrorStrings::kLocalScheduler};
        }
    }

    if (argsMod.timer.isTimeUp()) {
        LOG_W("Cocktail: Time limit reached");
        return {{}, false, BroadcastLineSolver::ErrorStrings::kTimeOut};
    }

    return {std::move(moduleResults), converged, ""};
}

std::tuple<std::vector<ProductionLineSolution>, nlohmann::json>
solve(problem::ProductionLine &problemInstance, const cli::CLIArgs &args) {
    uint64_t iterations = 0;
    const auto argsMod = cli::ModularArgs::fromArgs(args);

    auto &modules = problemInstance.modules();
    bool convergedLowerBound = false;
    AlgorithmsData algorithmsData;

    // Generate the delay graphs of each module
    for (auto &[moduleId, module] : modules) {
        module.updateDelayGraph(cg::Builder::build(module));
    }

    problem::ModuleId moduleId = problemInstance.getFirstModuleId();
    std::string globalErrorStr;
    DistributedSchedulerHistory history(argsMod.storeSequence, argsMod.storeBounds);

    while (iterations < argsMod.maxIterations && argsMod.timer.isRunning() && globalErrorStr.empty()) {
        auto [moduleResults, converged, errorStr] = singleIteration(
                problemInstance, args, iterations, convergedLowerBound, argsMod, history);

        if (!errorStr.empty()) {
            globalErrorStr = std::move(errorStr);
            break;
        }

        ++iterations;

        if (converged && convergedLowerBound) {
            return {{BroadcastLineSolver::mergeSolutions(problemInstance, moduleResults)},
                    BroadcastLineSolver::baseResultData(history, problemInstance, iterations)};
        }

        convergedLowerBound |= converged;
    }

    auto data = BroadcastLineSolver::baseResultData(history, problemInstance, iterations);
    data["timeout"] = argsMod.timer.isTimeUp();
    if (!globalErrorStr.empty()) {
        data["error"] = globalErrorStr;
    } else {
        data["error"] = BroadcastLineSolver::ErrorStrings::kNoConvergence;
    }

    return {ProductionLineSolutions{}, std::move(data)};
}

} // namespace fms::solvers::CocktailLineSolver
