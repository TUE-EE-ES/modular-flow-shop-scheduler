#include <gtest/gtest.h>

#include "test_utils/runner.hpp"

TEST(Maintenance, MIBHCSResult10) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::MIBHCS;
    args.maintPolicyFile = "maintenance/maintproperties.xml";
    auto solutions = TestUtils::runShop(args, "maintenance/result1_0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(Maintenance, MISIMSResult10) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::MISIM;
    args.maintPolicyFile = "maintenance/maintproperties.xml";
    auto solutions = TestUtils::runShop(args, "maintenance/result1_0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(Maintenance, MIBHCSRresult11) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::MIBHCS;
    args.maintPolicyFile = "maintenance/maintproperties.xml";
    auto solutions = TestUtils::runShop(args, "maintenance/result1_1.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(Maintenance, MISIMRresult11) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::MISIM;
    args.maintPolicyFile = "maintenance/maintproperties.xml";
    auto solutions = TestUtils::runShop(args, "maintenance/result1_1.xml");
    EXPECT_GT(solutions.size(), 0);
}

