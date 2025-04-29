#include <gtest/gtest.h>

#include "test_utils/runner.hpp"

TEST(BHCS, simple0) {
    fms::cli::CLIArgs args{.algorithm = fms::cli::AlgorithmType::BHCS};
    const auto [solutions, problem, _] = TestUtils::runShopFullDetails(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);

    const auto makespan = solutions[0].getRealMakespan(problem);
    EXPECT_EQ(makespan, 540);
}
