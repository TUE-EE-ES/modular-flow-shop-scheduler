#include <exception>
#include <gtest/gtest.h>

#include "test_utils/runner.hpp"

/*
// Backtrack has been disabled as the input file format changed and there was not enough time to
// reimplement this feature. It was used to compare the distributed scheduling approach with an 
// "all-knowing" system and we decided to do that through Constraint Programming

TEST(Backtrack, modular10) {
    CLIArgs args;
    args.modularAlgorithm = ModularAlgorithm::BACKTRACK;
    auto [solutions, _] = TestUtils::runLine(args, "modular/synthetic/1/0.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(Backtrack, bookletB10) {
    CLIArgs args;
    args.modularAlgorithm = ModularAlgorithm::BACKTRACK;
    auto [solutions, _] = TestUtils::runLine(args, "modular/printer_cases/bookletB/10.xml");
    EXPECT_GT(solutions.size(), 0);
}

TEST(Backtrack, bookletA0) {
    CLIArgs args;
    args.modularAlgorithm = ModularAlgorithm::BACKTRACK;
    auto [solutions, _] = TestUtils::runLine(args, "modular/printer_cases/bookletA/0.xml");
    EXPECT_GT(solutions.size(), 0);
}
*/
