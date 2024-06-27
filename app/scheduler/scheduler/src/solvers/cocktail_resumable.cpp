#include "pch/containers.hpp" // Precompiled headers always go first

#include "solvers/cocktail_resumable.hpp"

#include "FORPFSSPSD/indices.hpp"
#include "delayGraph/builder.hpp"
#include "solvers/broadcast_line_solver.hpp"
#include "solvers/cocktail_line_solver.hpp"
#include "solvers/modular_args.hpp"
#include "solvers/solver.h"

#include <fmt/compile.h>

using namespace algorithm;
using namespace algorithm::BroadcastLineSolver;

std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
CocktailResumableSolver::solve(FS::ProductionLine &problemInstance, const commandLineArgs &args) {
    uint64_t iterations = 0;
    const auto argsMod = ModularArgs::fromArgs(args);

    auto &modules = problemInstance.modules();
    bool convergedLowerBound = false;
    FMS::DistributedSchedulerHistory history(argsMod.storeSequence, argsMod.storeBounds);

    // Generate the delay graphs of each module
    for (auto &[moduleId, module] : modules) {
        module.updateDelayGraph(DelayGraph::Builder::build(module));
    }

    FS::ModuleId moduleId = problemInstance.getFirstModuleId();
    std::string globalErrorStr;

    while (iterations < args.maxIterations && argsMod.timer.isRunning() && globalErrorStr.empty()) {
        auto [moduleResults, converged, errorStr] =
                CocktailLineSolver::singleIteration(
                        problemInstance, args, iterations, convergedLowerBound, argsMod, history);

        if (!errorStr.empty()) {
            globalErrorStr = std::move(errorStr);
            break;
        }

        if (converged && convergedLowerBound) {
            return {{mergeSolutions(problemInstance, moduleResults)},
                    baseResultData(history, problemInstance, iterations)};
        }

        convergedLowerBound |= converged;
        ++iterations;
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
