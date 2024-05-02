#include <gtest/gtest.h>

#include "utils/runner.h"


TEST(DD, simple0FixedOrder) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::DD;
    
    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(DD, simple0FlowShop) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::DD;
    args.shopType = ShopType::FLOWSHOP;
    
    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(DD, simple0JobShop) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::DD;
    args.explorationType = DDExplorationType::STATIC;
    args.shopType = ShopType::JOBSHOP;
    
    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(DD, simple0SeedFixedOrder) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::DD;
    args.sequenceFile = "simple/0.seq.json";

    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(DD, simple0SeedFlowShop) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::DD;
    args.shopType = ShopType::FLOWSHOP;
    args.sequenceFile = "simple/0.seq.json";

    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(DD, simple0SeedJobShop) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::DD;
    args.shopType = ShopType::JOBSHOP;
    args.sequenceFile = "simple/0.seq.json";

    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}


