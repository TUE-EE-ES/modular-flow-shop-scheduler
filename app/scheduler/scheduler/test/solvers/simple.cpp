#include <gtest/gtest.h>

#include "test_utils/runner.hpp"

TEST(Simple, simple0) {
    fms::cli::CLIArgs args{.algorithm = fms::cli::AlgorithmType::SIMPLE};
    const auto [solutions, problem, _] = TestUtils::runShopFullDetails(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);

    const auto jobLast = problem.getJobsOutput().back();
    const auto lastOp = problem.jobs(jobLast).back();
    const auto makespan = solutions[0].getMakespan() + problem.getProcessingTime(lastOp);

    const auto makespanReal = solutions[0].getRealMakespan(problem);
    EXPECT_EQ(makespan, makespanReal);
    EXPECT_EQ(makespan, 870);
}
