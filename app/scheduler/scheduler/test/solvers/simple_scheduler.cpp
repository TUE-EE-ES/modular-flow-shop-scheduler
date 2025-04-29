#include <gtest/gtest.h>

#include "test_utils/runner.hpp"


TEST(SimpleScheduler, simple0) {
    fms::cli::CLIArgs args{.algorithm = fms::cli::AlgorithmType::SIMPLE};
    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 840);
}

