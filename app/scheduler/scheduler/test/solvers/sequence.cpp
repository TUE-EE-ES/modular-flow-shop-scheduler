#include <gtest/gtest.h>

#include "test_utils/runner.hpp"

#include <fms/problem/xml_parser.hpp>

TEST(Sequence, simple) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::GIVEN_SEQUENCE;
    args.sequenceFile = "simple/0.seq.json";
    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 840);
}

TEST(Sequence, simpleMultiPlexity) {
    fms::cli::CLIArgs args;
    args.algorithm = fms::cli::AlgorithmType::GIVEN_SEQUENCE;
    args.sequenceFile = "simple/1.seq.json";
    auto solutions = TestUtils::runShop(args, "simple/1.xml");
    EXPECT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 560);
}
