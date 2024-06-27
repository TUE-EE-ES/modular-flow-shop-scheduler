#include <gtest/gtest.h>

#include "utils/runner.h"


TEST(SimpleScheduler, simple0) {
    commandLineArgs args{.algorithm = AlgorithmType::SIMPLE};
    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 840);
}

