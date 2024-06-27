#include <gtest/gtest.h>

#include "utils/runner.h"

#include <FORPFSSPSD/boundary.hpp>
#include <FORPFSSPSD/bounds.hpp>
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
    EXPECT_EQ(solutions[0].getMakespan(), 1080);
}

TEST_F(Modular, modular10Cocktail) {
    m_args.modularAlgorithm = ModularAlgorithmType::COCKTAIL;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/synthetic/1/0.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 1230);
}

TEST_F(Modular, bookletB10Broadcast) {
    m_args.modularAlgorithm = ModularAlgorithmType::BROADCAST;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/printer_cases/bookletB/10.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 79802388);
}

TEST_F(Modular, bookletB10Cocktail) {
    m_args.modularAlgorithm = ModularAlgorithmType::COCKTAIL;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/printer_cases/bookletB/10.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 79802388);
}

TEST_F(Modular, bookletA0Broadcast) {
    m_args.modularAlgorithm = ModularAlgorithmType::BROADCAST;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/printer_cases/bookletA/0.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 57196882);
}

TEST_F(Modular, bookletA0Cocktail) {
    m_args.modularAlgorithm = ModularAlgorithmType::COCKTAIL;
    auto [solutions, _] = TestUtils::runLine(m_args, "modular/printer_cases/bookletA/0.xml");
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 57196882);
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

    GlobalBounds bounds{
            {ModuleId(0),
             {{},
              {{FS::JobId(0),
                {{FS::JobId(1), {100, 1000}},
                 {FS::JobId(2), {100, 1000}},
                 {FS::JobId(3), {100, 1000}},
                 {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(1),
                {{FS::JobId(2), {100, 1000}},
                 {FS::JobId(3), {100, 1000}},
                 {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(2), {{FS::JobId(3), {100, 1000}}, {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(3), {{FS::JobId(4), {100, 1000}}}}}}},
            {ModuleId(1),
             {{{FS::JobId(0),
                {{FS::JobId(1), {100, 1000}},
                 {FS::JobId(2), {100, 1000}},
                 {FS::JobId(3), {100, 1000}},
                 {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(1),
                {{FS::JobId(2), {100, 1000}},
                 {FS::JobId(3), {100, 1000}},
                 {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(2), {{FS::JobId(3), {100, 1000}}, {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(3), {{FS::JobId(4), {100, 1000}}}}},
              {}}}};

    const auto [translated, converged] = BroadcastLineSolver::translateBounds(line, bounds);
    EXPECT_TRUE(converged);

    bounds[ModuleId(0)].out.at(FS::JobId(0)).at(FS::JobId(1)) = {100, 1001};

    const auto [translated2, converged2] = BroadcastLineSolver::translateBounds(line, bounds);
    EXPECT_FALSE(converged2);
}

TEST_F(Modular, convergenceDetectionNull) {
    using namespace FORPFSSPSD;
    auto parser = TestUtils::checkArguments(m_args, "modular/synthetic/1/0.xml");
    auto line = parser.createProductionLine();

    const GlobalBounds bounds{
            {ModuleId(0),
             {{},
              {{FS::JobId(0),
                {{FS::JobId(1), {100, 1000}},
                 {FS::JobId(2), {100, 1000}},
                 {FS::JobId(3), {100, 1000}},
                 {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(1),
                {{FS::JobId(2), {100, 1000}},
                 {FS::JobId(3), {100, 1000}},
                 {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(2), {{FS::JobId(3), {100, 1000}}, {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(3), {{FS::JobId(4), {100, 1000}}}}}}},
            {ModuleId(1),
             {{{FS::JobId(0),
                {{FS::JobId(1), TimeInterval::Empty()},
                 {FS::JobId(2), TimeInterval::Empty()},
                 {FS::JobId(3), TimeInterval::Empty()},
                 {FS::JobId(4), TimeInterval::Empty()}}},
               {FS::JobId(1),
                {{FS::JobId(2), {100, 1000}},
                 {FS::JobId(3), {100, 1000}},
                 {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(2), {{FS::JobId(3), {100, 1000}}, {FS::JobId(4), {100, 1000}}}},
               {FS::JobId(3), {{FS::JobId(4), {100, 1000}}}}},
              {}}}};

    const auto [translated, converged] = BroadcastLineSolver::translateBounds(line, bounds);
    EXPECT_TRUE(converged);
}

TEST_F(Modular, saveAndRestoreBounds) {
    using namespace FORPFSSPSD;

    std::vector<GlobalBounds> intervals;

    for (std::size_t iters = 0; iters < 3; ++iters) {
        GlobalBounds global;
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

    nlohmann::json json = FORPFSSPSD::toJSON(intervals);
    auto restored = FORPFSSPSD::allGlobalBoundsFromJSON(json);

    EXPECT_EQ(intervals, restored);
}

TEST_F(Modular, cocktailConvergence) {
    // This test checks whether the convergence process of cocktail is correct. It was not
    // and we were always "converging" in 2 iterations :/
    using namespace FORPFSSPSD;

    m_args.modularAlgorithm = ModularAlgorithmType::COCKTAIL;
    constexpr const std::string_view file = "modular/printer_cases/bookletABUniform/54.xml";
    auto [solutions, data] = TestUtils::runLine(m_args, file);
    ASSERT_GT(solutions.size(), 0);
    EXPECT_EQ(solutions[0].getMakespan(), 95687606);
    EXPECT_EQ(data["iterations"], 3);
}

// NOLINTEND(*-magic-numbers,*-non-private-*)
