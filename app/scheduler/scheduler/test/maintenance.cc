#include <exception>
#include <gtest/gtest.h>

#include "utils/runner.h"

TEST(Maintenance, MIBHCSResult10) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::MIBHCS;
    args.maintPolicyFile = "maintenance/maintproperties.xml";
    auto solutions = TestUtils::runShop(args, "maintenance/result1_0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(Maintenance, MISIMSResult10) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::MISIM;
    args.maintPolicyFile = "maintenance/maintproperties.xml";
    auto solutions = TestUtils::runShop(args, "maintenance/result1_0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(Maintenance, MIBHCSRresult11) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::MIBHCS;
    args.maintPolicyFile = "maintenance/maintproperties.xml";
    auto solutions = TestUtils::runShop(args, "maintenance/result1_1.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(Maintenance, MISIMRresult11) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::MISIM;
    args.maintPolicyFile = "maintenance/maintproperties.xml";
    auto solutions = TestUtils::runShop(args, "maintenance/result1_1.xml");
    EXPECT_GT(solutions.size(), 0);
}

