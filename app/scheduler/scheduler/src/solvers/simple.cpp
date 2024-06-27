#include "pch/containers.hpp"

#include "solvers/simple.hpp"

#include "solvers/utils.hpp"

using namespace DelayGraph;
using namespace algorithm;
using namespace algorithm::SimpleScheduler;

SolverOutput algorithm::SimpleScheduler::solve(FORPFSSPSD::Instance &problemInstance,
                                               const commandLineArgs &args) {
    LOG("SimpleScheduler: Solving problem instance");

    SolversUtils::initProblemGraph(problemInstance);
    auto solution = SolversUtils::createTrivialSolution(problemInstance);

    // Return the solutions and the JSON object
    return {{std::move(solution)}, {}};
}
