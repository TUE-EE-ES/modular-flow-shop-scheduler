#include <gtest/gtest.h>

#include <fms/scheduler.hpp>
#include <fms/cli/command_line.hpp>

using namespace fms;

TEST(AlgorithmSelection, singleAlgorithm) {
    const cli::AlgorithmType selectedAlgorithm = cli::AlgorithmType::GIVEN_SEQUENCE;

    cli::CLIArgs args{
            .algorithm = selectedAlgorithm,
            .algorithms = {selectedAlgorithm},
            .multiAlgorithmBehaviour = cli::MultiAlgorithmBehaviour::FIRST,
    };

    constexpr const std::size_t numModules = 10;

    for (problem::ModuleId::ValueType i = 0; i < numModules; ++i) {
        const auto algorithm = Scheduler::getAlgorithm(
                problem::ModuleId{i}, args.algorithms.size(), numModules, args);
        EXPECT_EQ(algorithm, selectedAlgorithm);
    }
}

TEST(AlgorithmSelection, divideEven) {
    cli::CLIArgs args{
            .algorithms = {cli::AlgorithmType::GIVEN_SEQUENCE,
                           cli::AlgorithmType::MNEH,
                           cli::AlgorithmType::BHCS},
            .multiAlgorithmBehaviour = cli::MultiAlgorithmBehaviour::DIVIDE,
    };
    constexpr const std::size_t numModules = 9;
    for (problem::ModuleId::ValueType i = 0; i < numModules; ++i) {
        const auto algorithm = Scheduler::getAlgorithm(
                problem::ModuleId{i}, args.algorithms.size(), numModules, args);

        if (i < 3) {
            EXPECT_EQ(algorithm, cli::AlgorithmType::GIVEN_SEQUENCE);
        } else if (i < 6) {
            EXPECT_EQ(algorithm, cli::AlgorithmType::MNEH);
        } else {
            EXPECT_EQ(algorithm, cli::AlgorithmType::BHCS);
        }
    }
}

TEST(AlgorithmSelection, divideOdd) {
    cli::CLIArgs args{
            .algorithms = {cli::AlgorithmType::GIVEN_SEQUENCE,
                           cli::AlgorithmType::MNEH,
                           cli::AlgorithmType::BHCS},
            .multiAlgorithmBehaviour = cli::MultiAlgorithmBehaviour::DIVIDE,
    };
    constexpr const std::size_t numModules = 10;
    for (problem::ModuleId::ValueType i = 0; i < numModules; ++i) {
        const auto algorithm = Scheduler::getAlgorithm(
                problem::ModuleId{i}, args.algorithms.size(), numModules, args);
        if (i < 4) {
            EXPECT_EQ(algorithm, cli::AlgorithmType::GIVEN_SEQUENCE);
        } else if (i < 7) {
            EXPECT_EQ(algorithm, cli::AlgorithmType::MNEH);
        } else {
            EXPECT_EQ(algorithm, cli::AlgorithmType::BHCS);
        }
    }

    constexpr const std::size_t numModules2 = 11;
    for (problem::ModuleId::ValueType i = 0; i < numModules2; ++i) {
        const auto algorithm = Scheduler::getAlgorithm(
                problem::ModuleId{i}, args.algorithms.size(), numModules2, args);
        if (i < 4) {
            EXPECT_EQ(algorithm, cli::AlgorithmType::GIVEN_SEQUENCE);
        } else if (i < 8) {
            EXPECT_EQ(algorithm, cli::AlgorithmType::MNEH);
        } else {
            EXPECT_EQ(algorithm, cli::AlgorithmType::BHCS);
        }
    }
}

TEST(AlgorithmSelection, interleave) {
    cli::CLIArgs args{
            .algorithms = {cli::AlgorithmType::GIVEN_SEQUENCE,
                           cli::AlgorithmType::MNEH,
                           cli::AlgorithmType::BHCS},
            .multiAlgorithmBehaviour = cli::MultiAlgorithmBehaviour::INTERLEAVE,
    };
    constexpr const std::size_t numModules = 10;
    for (problem::ModuleId::ValueType i = 0; i < numModules; ++i) {
        const auto algorithm = Scheduler::getAlgorithm(
                problem::ModuleId{i}, args.algorithms.size(), numModules, args);
        if (i % 3 == 0) {
            EXPECT_EQ(algorithm, cli::AlgorithmType::GIVEN_SEQUENCE);
        } else if (i % 3 == 1) {
            EXPECT_EQ(algorithm, cli::AlgorithmType::MNEH);
        } else {
            EXPECT_EQ(algorithm, cli::AlgorithmType::BHCS);
        }
    }
}

TEST(AlgorithmSelection, first) {
    cli::CLIArgs args{
            .algorithms = {cli::AlgorithmType::GIVEN_SEQUENCE,
                           cli::AlgorithmType::MNEH,
                           cli::AlgorithmType::BHCS},
            .multiAlgorithmBehaviour = cli::MultiAlgorithmBehaviour::FIRST,
    };
    constexpr const std::size_t numModules = 10;
    for (problem::ModuleId::ValueType i = 0; i < numModules; ++i) {
        const auto algorithm = Scheduler::getAlgorithm(
                problem::ModuleId{i}, args.algorithms.size(), numModules, args);
        EXPECT_EQ(algorithm, cli::AlgorithmType::GIVEN_SEQUENCE);
    }
}

TEST(AlgorithmSelection, last) {
    cli::CLIArgs args{
            .algorithms = {cli::AlgorithmType::GIVEN_SEQUENCE,
                           cli::AlgorithmType::MNEH,
                           cli::AlgorithmType::BHCS},
            .multiAlgorithmBehaviour = cli::MultiAlgorithmBehaviour::LAST,
    };
    constexpr const std::size_t numModules = 10;
    for (problem::ModuleId::ValueType i = 0; i < numModules; ++i) {
        const auto algorithm = Scheduler::getAlgorithm(
                problem::ModuleId{i}, args.algorithms.size(), numModules, args);
        EXPECT_EQ(algorithm, cli::AlgorithmType::BHCS);
    }
}
