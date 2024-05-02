#include <gtest/gtest.h>

#include "utils/runner.h"

#include <FORPFSSPSD/boundary.hpp>
#include <FORPFSSPSD/xmlParser.h>
#include <filesystem>
#include <fmsschedulerexception.h>
#include <solvers/broadcast_line_solver.hpp>

#include <nlohmann/json.hpp>

// NOLINTBEGIN(*-magic-numbers,*-non-private-*)

using namespace algorithm;

class Modular : public ::testing::Test {
protected:
    void SetUp() override { m_cwd = std::filesystem::current_path(); }

    static void SetUp(const std::string &path) { std::filesystem::current_path(path); }

    void TearDown() override { std::filesystem::current_path(m_cwd); }

    std::filesystem::path m_cwd;
    commandLineArgs m_args;
};

TEST_F(Modular, modular10Broadcast) {
    m_args.modularAlgorithm = ModularAlgorithmType::BROADCAST;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/synthetic/1/0.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 1050);
}

TEST_F(Modular, modular10Cocktail) {
    m_args.modularAlgorithm = ModularAlgorithmType::COCKTAIL;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/synthetic/1/0.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 1200);
}

TEST_F(Modular, bookletB10Broadcast) {
    m_args.modularAlgorithm = ModularAlgorithmType::BROADCAST;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/printer_cases/bookletB/10.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 79144388);
}

TEST_F(Modular, bookletB10Cocktail) {
    m_args.modularAlgorithm = ModularAlgorithmType::COCKTAIL;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/printer_cases/bookletB/10.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 79144388);
}

TEST_F(Modular, bookletA0Broadcast) {
    m_args.modularAlgorithm = ModularAlgorithmType::BROADCAST;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/printer_cases/bookletA/0.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 56809882);
}

TEST_F(Modular, bookletA0Cocktail) {
    m_args.modularAlgorithm = ModularAlgorithmType::COCKTAIL;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/printer_cases/bookletA/0.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 56809882);
}

TEST_F(Modular, nonTerminating) {
    SetUp("modular/synthetic/non-terminating");
    m_args.modularAlgorithm = ModularAlgorithmType::BROADCAST;
    m_args.algorithm = AlgorithmType::GIVEN_SEQUENCE;
    m_args.maxIterations = 40;
    m_args.sequenceFile = "problem.seq.json";
    const std::string_view file = "problem.xml";

    auto [solutions, json] = TestUtils::runLine(m_args, file);
    EXPECT_EQ(json["iterations"], m_args.maxIterations);
    EXPECT_EQ(solutions.size(), 0);
}

TEST_F(Modular, convergenceDetection) {
    using namespace FORPFSSPSD;
    auto parser = TestUtils::checkArguments(m_args, "modular/synthetic/1/0.xml");
    auto line = parser.createProductionLine();

    algorithm::GlobalIntervals intervals{
            {ModuleId(0),
             {{},
              {{0, {{1, {100, 1000}}, {2, {100, 1000}}, {3, {100, 1000}}, {4, {100, 1000}}}},
               {1, {{2, {100, 1000}}, {3, {100, 1000}}, {4, {100, 1000}}}},
               {2, {{3, {100, 1000}}, {4, {100, 1000}}}},
               {3, {{4, {100, 1000}}}}}}},
            {ModuleId(1),
             {{{0, {{1, {100, 1000}}, {2, {100, 1000}}, {3, {100, 1000}}, {4, {100, 1000}}}},
               {1, {{2, {100, 1000}}, {3, {100, 1000}}, {4, {100, 1000}}}},
               {2, {{3, {100, 1000}}, {4, {100, 1000}}}},
               {3, {{4, {100, 1000}}}}},
              {}}}};

    const auto [translated, converged] = BroadcastLineSolver::translateBounds(line, intervals);
    EXPECT_TRUE(converged);

    intervals[ModuleId(0)].out.at(0).at(1) = {100, 1001};

    const auto [translated2, converged2] = BroadcastLineSolver::translateBounds(line, intervals);
    EXPECT_FALSE(converged2);
}

TEST_F(Modular, convergenceDetectionNull) {
    using namespace FORPFSSPSD;
    auto parser = TestUtils::checkArguments(m_args, "modular/synthetic/1/0.xml");
    auto line = parser.createProductionLine();

    algorithm::GlobalIntervals intervals{
            {ModuleId(0),
             {{},
              {{0, {{1, {100, 1000}}, {2, {100, 1000}}, {3, {100, 1000}}, {4, {100, 1000}}}},
               {1, {{2, {100, 1000}}, {3, {100, 1000}}, {4, {100, 1000}}}},
               {2, {{3, {100, 1000}}, {4, {100, 1000}}}},
               {3, {{4, {100, 1000}}}}}}},
            {ModuleId(1),
             {{{0,
                {{1, TimeInterval::Empty()},
                 {2, TimeInterval::Empty()},
                 {3, TimeInterval::Empty()},
                 {4, TimeInterval::Empty()}}},
               {1, {{2, {100, 1000}}, {3, {100, 1000}}, {4, {100, 1000}}}},
               {2, {{3, {100, 1000}}, {4, {100, 1000}}}},
               {3, {{4, {100, 1000}}}}},
              {}}}};

    const auto [translated, converged] = BroadcastLineSolver::translateBounds(line, intervals);
    EXPECT_TRUE(converged);
}

TEST_F(Modular, saveAndRestoreBounds) {
    using namespace FORPFSSPSD;

    std::vector<algorithm::GlobalIntervals> intervals;

    for (std::size_t iters = 0; iters < 3; ++iters) {
        algorithm::GlobalIntervals global;
        for (std::size_t i = 0; i < 4; ++i) {
            const ModuleId moduleId(i);
            for (std::size_t j1 = 0; j1 < 4; ++j1) {
                const JobId jobFrom(j1);
                for (std::size_t j2 = j1 + 1; j2 < 5; ++j2) {
                    const JobId jobTo(j2);

                    global[moduleId].in[jobFrom].insert(
                            {jobTo, {100 * (j1 + 1) + iters, 1000 * (j2 + 1) + iters}});
                    global[moduleId].out[jobFrom].insert(
                            {jobTo, {100 * (j1 + 1) + 50 + iters, 1000 * (j2 + 1) + 50 + iters}});
                }
            }
        }
        intervals.push_back(std::move(global));
    }

    nlohmann::json json = BroadcastLineSolver::boundsToJSON(intervals);
    auto restored = BroadcastLineSolver::allGlobalBoundsFromJSON(json);

    EXPECT_EQ(intervals, restored);
}

// NOLINTEND(*-magic-numbers,*-non-private-*)
