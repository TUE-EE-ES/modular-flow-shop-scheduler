#include <gtest/gtest.h>

#include "test_utils/instance_generator.hpp"

#include <fms/cg/builder.hpp>
#include <fms/cg/constraint_graph.hpp>
#include <fms/cg/export_utilities.hpp>
#include <fms/problem/flow_shop.hpp>
#include <fms/scheduler.hpp>
#include <fms/solvers/pareto_heuristic.hpp>
#include <fms/solvers/partial_solution.hpp>

#include <algorithm>

using namespace fms;
using namespace fms::solvers;

TEST(Pareto, TinyHomogeneousCase) {
    auto f = createHomogeneousCase(863, 456, 735, 774, 13958, 15395, 2);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    cg::exports::saveAsTikz(f, f.getDelayGraph(), "tiny_homogeneous.tex");
    cli::CLIArgs args;
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    auto solutions = ParetoHeuristic::solve(f, args);

    EXPECT_GE(solutions.size(), 1u);
}

TEST(Pareto, SmallHomogeneousCase) {
    auto f = createHomogeneousCase(863, 456, 735, 774, 13958, 15395, 50);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    cg::exports::saveAsTikz(f, f.getDelayGraph(), "small_homogeneous.tex");
    cli::CLIArgs args;
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    auto solutions = ParetoHeuristic::solve(f, args);

    EXPECT_GE(solutions.size(), 1u);

    args.outputFile = "___sol.txt";
    for (const auto &solution : solutions) {
        // Double check that the reported makespan makes sense
        delay makespan = solution.getMakespan();
        EXPECT_EQ(makespan, solution.getMakespan());
    }
}

TEST(Pareto, NoInterleavingPossible) {
    /** only possible to empty the loop everytime **/
    auto f = createHomogeneousCase(1, 1, 1, 1, 1, 1, 50);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    cli::CLIArgs args;
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    auto solutions = ParetoHeuristic::solve(f, args);
    cg::exports::saveAsTikz(f, solutions[0], "no-interleaving-possible.tex");

    // only one option, so there is only one solution (which is also feasible!)
    ASSERT_EQ(solutions.size(), 1u);
    delay makespan = solutions[0].getMakespan();
    ASSERT_EQ(makespan, 101); // 1 + 50 * 2 + 1 - 1 (starting time of last operation)
}

TEST(Pareto, AllFirstPassBeforeSecondPass) {
    /** optimal schedule is to do all first passes before any of the second passes start **/
    auto f = createHomogeneousCase(1, 10, 10, 1, 100, 150, 14);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    cli::CLIArgs args;

    args.maxPartialSolutions = 100; // Make sure we don't reduce
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    auto solutions = ParetoHeuristic::solve(f, args);

    ASSERT_GE(solutions.size(), 1u);
    delay minMakespan = std::numeric_limits<delay>::max();
    unsigned int i = 0;
    for (const auto &sol : solutions) {
        delay makespan = sol.getMakespan();
        minMakespan = std::min(minMakespan, makespan);
        cg::exports::saveAsTikz(
                f, sol, fmt::format("all-firstpass-before-secondpass{}.tex", i));
    }

    ASSERT_EQ(minMakespan, 281); // 1 + 13 * 10 * 2 + 10 + 1 - 1 (starting time of last operation)
}

TEST(Pareto, LongHomogeneousCase) {
    /** optimal schedule only depends on hiding processing time effectively in the buffer time **/
    auto f = createHomogeneousCase(1, 10, 10, 1, 100, 150, 52);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    cli::CLIArgs args;
    args.outputFile = "long-homogeneous-test.txt";
    args.maxPartialSolutions = 100;

    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    auto solutions = solvers::ParetoHeuristic::solve(f, args);

    ASSERT_GE(solutions.size(), 1u);
    delay makespan = solutions[0].getMakespan();
    cg::exports::saveAsTikz(
            f, solutions[0], "long-homogeneous-test.tex", solutions[0].getAllChosenEdges(f));

    std::cout << chosenSequencesToString(solutions[0]) << std::endl;

    ASSERT_EQ(makespan, 1041); // (starting time of last operation) 1 + 52 * 10 * 2 + 1 - 1 = 1041
}
