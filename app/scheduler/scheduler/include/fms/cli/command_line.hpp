// this file implements a command line parser using getopt()

#ifndef FMS_UTILS_COMMAND_LINE_HPP
#define FMS_UTILS_COMMAND_LINE_HPP

#include "fms/utils/logger.hpp"

#include "algorithm_type.hpp"
#include "dd_exploration_type.hpp"
#include "modular_algorithm_type.hpp"
#include "multi_algorithm_behaviour.hpp"
#include "schedule_output_format.hpp"
#include "shop_type.hpp"

namespace fms::cli {

struct CLIArgs {
    // mandatory
    std::string inputFile;
    std::string outputFile;
    std::string sequenceFile;

    // optional - Must be filled with defs.
    // NOLINTBEGIN
    std::string maintPolicyFile = "";
    utils::LOGGER_LEVEL verbose = utils::LOGGER_LEVEL::CRITICAL;
    double productivityWeight = 0.7;
    double flexibilityWeight = 0.25;
    double tieWeight = 0.05;
    std::chrono::milliseconds timeOut{5000};
    std::uint64_t maxIterations = std::numeric_limits<std::uint64_t>::max();
    std::uint32_t maxPartialSolutions = 5;
    AlgorithmType algorithm = AlgorithmType::BHCS;
    std::vector<AlgorithmType> algorithms = {AlgorithmType::BHCS};
    std::vector<std::string> algorithmOptions;
    ModularAlgorithmType modularAlgorithm = ModularAlgorithmType::BROADCAST;
    ScheduleOutputFormat outputFormat = ScheduleOutputFormat::JSON;
    ShopType shopType = ShopType::FIXEDORDERSHOP;
    DDExplorationType explorationType = DDExplorationType::STATIC;
    MultiAlgorithmBehaviour multiAlgorithmBehaviour = MultiAlgorithmBehaviour::DIVIDE;

    struct {
        bool storeBounds = false;
        bool storeSequence = false;
        bool noSelfBounds = false;
        std::uint64_t maxIterations = std::numeric_limits<std::uint64_t>::max();
        std::chrono::milliseconds timeOut{5000};
    } modularOptions;
    // NOLINTEND
};

/// @brief Extracts the command line arguments into the @ref CLIArgs struct.
CLIArgs getArgs(int argc, char **argv);
} // namespace fms::cli

#endif /* FMS_UTILS_COMMAND_LINE_HPP */
