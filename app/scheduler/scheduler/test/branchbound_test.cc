#include <gtest/gtest.h>

#include <FORPFSSPSD/FORPFSSPSD.h>
#include <delayGraph/builder.hpp>
#include <delayGraph/delayGraph.h>
#include <delayGraph/export_utilities.h>
#include <fmsscheduler.h>
#include <solvers/branchbound.h>

#include "instance_generator.h"

using namespace std;
using namespace DelayGraph;
using namespace FORPFSSPSD;

// NOLINTBEGIN(*-magic-numbers)


TEST(BRANCHBOUND, TinyHomogeneousCase) {
    auto f = createHomogeneousCase(863, 456, 735, 774, 13958, 15395, 2);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    auto solutions = algorithm::BranchBound::solve(f, commandLineArgs{});
}

TEST(BRANCHBOUND, SmallHomogeneousCase) {
    auto f = createHomogeneousCase(863, 456, 735, 774, 13958, 15395, 20);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    commandLineArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto best_solution = algorithm::BranchBound::solve(f, args);
    std::cout << best_solution.getMakespan() << std::endl;
    std::cout << f.getTrivialCompletionTimeLowerbound() << std::endl;
    EXPECT_GT(best_solution.getMakespan(), f.getTrivialCompletionTimeLowerbound());
    std::cout << chosenEdgesToString(best_solution, f.getDelayGraph()) << std::endl;
}


TEST(BRANCHBOUND, NoInterleavingPossible) {
    /** only possible to empty the loop everytime **/
    auto f = createHomogeneousCase(1, 1, 1, 1, 1, 1, 50);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    commandLineArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto solution = algorithm::BranchBound::solve(f, args);
    // 1 + 50 * 2 + 1 - 1 (starting time of last operation)
    ASSERT_EQ(solution.getMakespan(), 101);
}

TEST(BRANCHBOUND, NoInterleavingPossibleSmall) {
    /** only possible to empty the loop everytime **/
    auto f = createHomogeneousCase(1, 1, 1, 1, 1, 1, 5);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    commandLineArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto solution = algorithm::BranchBound::solve(f, args);
    // 1 + 5 * 2 + 1 - 1 (starting time of last operation)
    ASSERT_EQ(solution.getMakespan(), 11);
}

TEST(BRANCHBOUND, AllFirstPassBeforeSecondPass) {
    /** optimal schedule is to do all first passes before any of the second passes start **/
    auto f = createHomogeneousCase(1, 10, 10, 1, 100, 150, 14);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    commandLineArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto solutions = algorithm::BranchBound::solve(f, args);
    // 1 + 13 * 10 * 2 + 10 (starting time of last operation)
    ASSERT_EQ(solutions.getMakespan(), 281);
}

TEST(BRANCHBOUND, LongHomogeneousCaseFitsExactlyInMinBuffer) {
    /** optimal schedule is to have no flushing (so all interleaved), because that would result in a non-multiple of **/
    auto f = createHomogeneousCase(1, 10, 10, 1, 100, 150, 52);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    commandLineArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto solutions = algorithm::BranchBound::solve(f, args);
    // (starting time of last operation) = 1041 = 1 + 52 * 10 * 2
    ASSERT_EQ(solutions.getMakespan(), 1041);
}

TEST(BRANCHBOUND, LongHomogeneousCase) {
    /** optimal schedule is to have no flushing (so all interleaved), because that would result in a non-multiple of **/
    auto f = createHomogeneousCase(1, 10, 10, 1, 105, 150, 22);
    f.updateDelayGraph(Builder::FORPFSSPSD(f));
    ASSERT_TRUE(FmsScheduler::checkConsistency(f).first);

    commandLineArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto solutions = algorithm::BranchBound::solve(f, args);
    // (starting time of last operation) = 441 = 1 + 22 * 10 * 2
    ASSERT_GE(solutions.getMakespan(), 441);
}

// NOLINTEND(*-magic-numbers)