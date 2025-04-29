#include <gtest/gtest.h>

#include "test_utils/instance_generator.hpp"

#include <fms/cg/builder.hpp>
#include <fms/cg/constraint_graph.hpp>
#include <fms/cg/export_utilities.hpp>
#include <fms/problem/flow_shop.hpp>
#include <fms/scheduler.hpp>
#include <fms/solvers/branch_bound.hpp>

using namespace fms;
using namespace fms::solvers;

// NOLINTBEGIN(*-magic-numbers)

TEST(BranchBound, TinyHomogeneousCase) {
    auto f = createHomogeneousCase(863, 456, 735, 774, 13958, 15395, 2);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    auto solutions = solvers::branch_bound::solve(f, cli::CLIArgs{});
}

TEST(BranchBound, SmallHomogeneousCase) {
    auto f = createHomogeneousCase(863, 456, 735, 774, 13958, 15395, 20);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    cli::CLIArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto best_solution = solvers::branch_bound::solve(f, args);
    std::cout << best_solution.getMakespan() << std::endl;
    std::cout << branch_bound::createTrivialCompletionLowerBound(f) << std::endl;
    EXPECT_GT(best_solution.getMakespan(), branch_bound::createTrivialCompletionLowerBound(f));
    std::cout << chosenSequencesToString(best_solution) << std::endl;
}

TEST(BranchBound, NoInterleavingPossible) {
    /** only possible to empty the loop everytime **/
    auto f = createHomogeneousCase(1, 1, 1, 1, 1, 1, 50);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    cli::CLIArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto solution = solvers::branch_bound::solve(f, args);
    // 1 + 50 * 2 + 1 - 1 (starting time of last operation)
    ASSERT_EQ(solution.getMakespan(), 101);
}

TEST(BranchBound, NoInterleavingPossibleSmall) {
    /** only possible to empty the loop everytime **/
    auto f = createHomogeneousCase(1, 1, 1, 1, 1, 1, 5);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    cli::CLIArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto solution = solvers::branch_bound::solve(f, args);
    // 1 + 5 * 2 + 1 - 1 (starting time of last operation)
    ASSERT_EQ(solution.getMakespan(), 11);
}

TEST(BranchBound, AllFirstPassBeforeSecondPass) {
    /** optimal schedule is to do all first passes before any of the second passes start **/
    auto f = createHomogeneousCase(1, 10, 10, 1, 100, 150, 14);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    cli::CLIArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto solutions = solvers::branch_bound::solve(f, args);
    // 1 + 13 * 10 * 2 + 10 (starting time of last operation)
    ASSERT_EQ(solutions.getMakespan(), 281);
}

TEST(BranchBound, LongHomogeneousCaseFitsExactlyInMinBuffer) {
    /** optimal schedule is to have no flushing (so all interleaved), because that would result in a
     * non-multiple of **/
    auto f = createHomogeneousCase(1, 10, 10, 1, 100, 150, 52);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    cli::CLIArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto solutions = solvers::branch_bound::solve(f, args);
    // (starting time of last operation) = 1041 = 1 + 52 * 10 * 2
    ASSERT_EQ(solutions.getMakespan(), 1041);
}

TEST(BranchBound, LongHomogeneousCase) {
    /** optimal schedule is to have no flushing (so all interleaved), because that would result in a
     * non-multiple of **/
    auto f = createHomogeneousCase(1, 10, 10, 1, 105, 150, 22);
    f.updateDelayGraph(cg::Builder::FORPFSSPSD(f));
    ASSERT_TRUE(Scheduler::checkConsistency(f).first);

    cli::CLIArgs args;
    args.timeOut = std::chrono::seconds(f.getNumberOfJobs());
    auto solutions = solvers::branch_bound::solve(f, args);
    // (starting time of last operation) = 441 = 1 + 22 * 10 * 2
    ASSERT_GE(solutions.getMakespan(), 441);
}

// NOLINTEND(*-magic-numbers)
