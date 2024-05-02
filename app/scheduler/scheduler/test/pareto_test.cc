#include <gtest/gtest.h>

#include <FORPFSSPSD/FORPFSSPSD.h>
#include <algorithm>
#include <delayGraph/builder.hpp>
#include <delayGraph/delayGraph.h>
#include <delayGraph/export_utilities.h>
#include <fmsscheduler.h>
#include <partialsolution.h>
#include <solvers/paretoheuristic.h>

#include "instance_generator.h"

using namespace std;
using namespace DelayGraph;
using namespace FORPFSSPSD;


TEST(PARETO, TinyHomogeneousCase) {
    auto f = createHomogeneousCase(863, 456, 735, 774, 13958, 15395, 2);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    export_utilities::saveAsTikz(f, f.getDelayGraph(), "tiny_homogeneous.tex");
    commandLineArgs args;
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    auto solutions = algorithm::ParetoHeuristic::solve(f, args);

    EXPECT_GE(solutions.size(), 1u);

}

TEST(PARETO, SmallHomogeneousCase) {
    auto f = createHomogeneousCase(863, 456, 735, 774, 13958, 15395, 50);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    export_utilities::saveAsTikz(f, f.getDelayGraph(), "small_homogeneous.tex");
    commandLineArgs args;
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    auto solutions = algorithm::ParetoHeuristic::solve(f, args);

    EXPECT_GE(solutions.size(), 1u);

    args.outputFile = "___sol.txt";
    for(const auto & solution : solutions) {
        // Double check that the reported makespan makes sense
        delay makespan = solution.getMakespan();
        EXPECT_EQ(makespan, solution.getMakespan());
    }
}

TEST(PARETO, NoInterleavingPossible) {
    /** only possible to empty the loop everytime **/
    auto f = createHomogeneousCase(1, 1, 1, 1, 1, 1, 50);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    commandLineArgs args;
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    auto solutions = algorithm::ParetoHeuristic::solve(f, args);
    DelayGraph::export_utilities::saveAsTikz(f, solutions[0], "no-interleaving-possible.tex");

    // only one option, so there is only one solution (which is also feasible!)
    ASSERT_EQ(solutions.size(), 1u);
    delay makespan = solutions[0].getMakespan();
    ASSERT_EQ(makespan, 101); // 1 + 50 * 2 + 1 - 1 (starting time of last operation)

}


TEST(PARETO, AllFirstPassBeforeSecondPass) {
    /** optimal schedule is to do all first passes before any of the second passes start **/
    auto f = createHomogeneousCase(1, 10, 10, 1, 100, 150, 14);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    commandLineArgs args;

    args.maxPartialSolutions = 100; // Make sure we don't reduce
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    auto solutions = algorithm::ParetoHeuristic::solve(f, args);

    ASSERT_GE(solutions.size(), 1u);
    delay minMakespan = std::numeric_limits<delay>::max();
    unsigned int i = 0;
    for(const auto& sol : solutions) {
        delay makespan = sol.getMakespan();
        minMakespan = std::min(minMakespan, makespan);
        DelayGraph::export_utilities::saveAsTikz(
                f, sol, fmt::format("all-firstpass-before-secondpass{}.tex", i));
    }

    ASSERT_EQ(minMakespan, 281); // 1 + 13 * 10 * 2 + 10 + 1 - 1 (starting time of last operation)
}

TEST(PARETO, LongHomogeneousCase) {
    /** optimal schedule only depends on hiding processing time effectively in the buffer time **/
    auto f = createHomogeneousCase(1, 10, 10, 1, 100, 150, 52);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    commandLineArgs args;
    args.outputFile = "long-homogeneous-test.txt";
    args.maxPartialSolutions = 100;

    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    auto solutions = algorithm::ParetoHeuristic::solve(f, args);

    ASSERT_GE(solutions.size(), 1u);
    delay makespan = solutions[0].getMakespan();
    DelayGraph::export_utilities::saveAsTikz(f, solutions[0], "long-homogeneous-test.tex", solutions[0].getAllChosenEdges());

    std::cout << chosenEdgesToString(solutions[0], f.getDelayGraph()) << std::endl;

    ASSERT_EQ(makespan, 1041); // (starting time of last operation) 1 + 52 * 10 * 2 + 1 - 1 = 1041
}
