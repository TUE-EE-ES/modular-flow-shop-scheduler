#include "fms/pch/containers.hpp"

#include "fms/solvers/simple.hpp"

#include "fms/solvers/utils.hpp"

using namespace fms;
using namespace fms::solvers;

SolverOutput SimpleScheduler::solve(problem::Instance &problemInstance,
                                    const cli::CLIArgs &args) {
    LOG("SimpleScheduler: Solving problem instance");

    SolversUtils::initProblemGraph(problemInstance);
    auto solution = SolversUtils::createTrivialSolution(problemInstance);

    // Return the solutions and the JSON object
    return {{std::move(solution)}, nlohmann::json::object()};
}
