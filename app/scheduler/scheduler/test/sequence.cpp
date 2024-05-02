#include <gtest/gtest.h>

#include "utils/runner.h"

#include <FORPFSSPSD/xmlParser.h>

TEST(Sequence, simple) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::GIVEN_SEQUENCE;
    args.sequenceFile = "simple/0.seq.json";
    auto solutions = TestUtils::runShop(args, "simple/0.xml");
    EXPECT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 840);
}

TEST(Sequence, simpleMultiPlexity) {
    commandLineArgs args;
    args.algorithm = AlgorithmType::GIVEN_SEQUENCE;
    args.sequenceFile = "simple/1.seq.json";
    auto solutions = TestUtils::runShop(args, "simple/1.xml");
    EXPECT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 560);
}

