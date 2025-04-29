#include <gtest/gtest.h>

#include "test_utils/runner.hpp"


TEST(DD, simple0FixedOrder) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::DD;
    
    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(DD, simple0FlowShop) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::DD;
    args.shopType = fms::cli::ShopType::FLOWSHOP;
    
    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(DD, simple0JobShop) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::DD;
    args.explorationType = fms::cli::DDExplorationType::STATIC;
    args.shopType = fms::cli::ShopType::JOBSHOP;
    
    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(DD, simple0SeedFixedOrder) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::DD;
    args.sequenceFile = "simple/0.seq.json";

    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(DD, simple0SeedFlowShop) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::DD;
    args.shopType = fms::cli::ShopType::FLOWSHOP;
    args.sequenceFile = "simple/0.seq.json";

    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(DD, simple0SeedJobShop) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::DD;
    args.shopType = fms::cli::ShopType::JOBSHOP;
    args.sequenceFile = "simple/0.seq.json";

    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
}


