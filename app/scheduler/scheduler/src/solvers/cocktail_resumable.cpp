#include "fms/pch/containers.hpp" // Precompiled headers always go first
#include "fms/pch/fmt.hpp"

#include "fms/solvers/cocktail_resumable.hpp"

#include "fms/cg/builder.hpp"
#include "fms/problem/indices.hpp"
#include "fms/solvers/broadcast_line_solver.hpp"
#include "fms/solvers/cocktail_line_solver.hpp"
#include "fms/solvers/modular_args.hpp"
#include "fms/solvers/solver.hpp"

using namespace fms;
using namespace fms::solvers;
using namespace fms::solvers::cocktail_resumable;

std::tuple<std::vector<ProductionLineSolution>, nlohmann::json>
solve(problem::ProductionLine &problemInstance, const cli::CLIArgs &args) {
    uint64_t iterations = 0;
    const auto argsMod = cli::ModularArgs::fromArgs(args);

    auto &modules = problemInstance.modules();
    bool convergedLowerBound = false;
    DistributedSchedulerHistory history(argsMod.storeSequence, argsMod.storeBounds);

    // Generate the delay graphs of each module
    for (auto &[moduleId, module] : modules) {
        module.updateDelayGraph(cg::Builder::build(module));
    }

    problem::ModuleId moduleId = problemInstance.getFirstModuleId();
    std::string globalErrorStr;

    while (iterations < argsMod.maxIterations && argsMod.timer.isRunning() && globalErrorStr.empty()) {
        auto [moduleResults, converged, errorStr] = CocktailLineSolver::singleIteration(
                problemInstance, args, iterations, convergedLowerBound, argsMod, history);

        if (!errorStr.empty()) {
            globalErrorStr = std::move(errorStr);
            break;
        }

        if (converged && convergedLowerBound) {
            return {{BroadcastLineSolver::mergeSolutions(problemInstance, moduleResults)},
                    BroadcastLineSolver::baseResultData(history, problemInstance, iterations)};
        }

        convergedLowerBound |= converged;
        ++iterations;
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
