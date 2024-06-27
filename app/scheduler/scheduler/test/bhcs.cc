#include <gtest/gtest.h>

#include "utils/runner.h"


TEST(BHCS, simple0) {
    commandLineArgs args{.algorithm = AlgorithmType::BHCS};
    const auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

