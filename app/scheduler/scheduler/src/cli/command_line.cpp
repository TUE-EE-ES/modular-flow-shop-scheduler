/*
 * Author		: Umar Waqas
 * Date			: 10-07-2013
 * Email		: u.waqas@tue.nl

 * License notice:
 * The licensing of this software is in progress.
 *
 * Description	:
 * Base class for Commandline parsing.
 *
 */

#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/cli/command_line.hpp"

#include "fms/utils/strings.hpp"
#include "fms/versioning.hpp"

#include <algorithm>
#include <cstdlib>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>

// NOLINTBEGIN(concurrency-mt-unsafe): This is only single threaded code

namespace fms::cli {

namespace {
std::tuple<cxxopts::Options, CLIArgs> getOptions() {
    cxxopts::Options options("fms-scheduler", "A Heuristic based Constraint Scheduler");
    CLIArgs args;

    // clang-format off
    options.add_options()
        ("h,help", "Show this help")
        ("version", "Show version")
        ("i,input", "Input file", cxxopts::value<std::string>())
        ("o,output", "Output file", cxxopts::value<std::string>())
        ("m,maintenance", "Maintenance policy file",
            cxxopts::value<std::string>()->default_value(""))
        ("v,verbose", "Verbose (use logging)")
        ("p,productivity", "Productivity",
            cxxopts::value<double>()->default_value(std::to_string(args.productivityWeight)))
        ("f,flexibility", "Flexibility",
            cxxopts::value<double>()->default_value(std::to_string(args.flexibilityWeight)))
        ("t,tie", "Tie", cxxopts::value<double>()->default_value(std::to_string(args.tieWeight)))
        ("time-out", "Time Out for anytime heuristic in miliseconds", 
            cxxopts::value<std::int64_t>()->default_value(std::to_string(args.timeOut.count())))
        ("k,max-partial", "Maximum of partial solutions to keep in Pareto algorithm",
            cxxopts::value<std::uint32_t>()->default_value(std::to_string(args.maxPartialSolutions)))
        ("a,algorithm", "Algorithm to use (bhcs|mdbhcs|pareto...) Use --list-algorithms to list all"
            " available algorithms.",
            cxxopts::value<std::vector<std::string>>()->default_value(std::string{args.algorithm.shortName()}))
        ("r,output-format", "Output format (json|cbor)",
            cxxopts::value<std::string>()->default_value(std::string{args.outputFormat.shortName()}))
        ("s,sequence-file", "Re-entrant machine operation sequence file",
            cxxopts::value<std::string>()->default_value(args.sequenceFile))
        ("max-iterations", "Maximum number of iterations that the algorithm should perform",
            cxxopts::value<std::uint64_t>()->default_value(std::to_string(args.maxIterations)))
        ("modular-algorithm", "Algorithm to use for modular scheduling (broadcast|cocktail|broadcast-half|cocktail-half).", 
            cxxopts::value<std::string>()->default_value(std::string{args.modularAlgorithm.shortName()}))
        ("modular-store-bounds", "Store the bounds of every iteration in the output JSON.")
        ("modular-store-sequence", "Store the sequence used at every iteration of the "
            "modular algorithm in the JSON output.")
        ("modular-no-self-bounds", "Do not store the bounds that a module sends in the "
            "module itself (may increase convergence time).")
        ("modular-multi-algorithm-behaviour,modular-multi-algorithm-behavior", "Behaviour of the "
            "modular algorithm when multiple local algorithms are specified.",
            cxxopts::value<std::string>()->default_value(std::string{args.multiAlgorithmBehaviour.shortName()}))
        ("modular-max-iterations", "Maximum number of iterations that the modular algorithm can perform.",
            cxxopts::value<std::uint64_t>()->default_value(std::to_string(args.modularOptions.maxIterations)))
        ("modular-time-out", "Time Out for modular algorithm in miliseconds",
            cxxopts::value<std::int64_t>()->default_value(std::to_string(args.modularOptions.timeOut.count())))
        ("shop-type", "Tell the SAG solution what type of shop it is solving.\n"
            "Accepted options are: 'flow','job' or 'fixedorder'",
            cxxopts::value<std::string>()->default_value(std::string{args.shopType.shortName()}))
        ("exploration-type", "Tell the DD solution what type of graph exploration technique it should use.\n"
            "Accepted options are: 'breadth','depth', 'best','static' or 'adaptive'",
            cxxopts::value<std::string>()->default_value(std::string{args.explorationType.shortName()}))
        ("list-algorithms", "List all available algorithms and exit")
        ("list-modular-algorithms", "List all available modular algorithms and exit")
        ("list-modular-multi-algorithm-behaviour,list-modular-multi-algorithm-behavior", 
            "List all available modular multi-algorithm behaviours and exit")
    ;
    // clang-format on

    options.parse_positional({"input", "output"});
    options.positional_help("input output");
    options.show_positional_help();

    return {std::move(options), std::move(args)};
}

/* print the usage of the tool */
void printUsage(const cxxopts::Options &options) {
    fmt::print(std::cout, "{}\nVersion information: {}\n", options.help(), VERSION);
}

void printAlgorithm(const std::string_view name,
                    const std::string_view fullName,
                    const std::string_view description,
                    const std::size_t maxWidth = 60) {
    constexpr const std::size_t indent = 2;

    if (fullName.empty()) {
        fmt::println("\033[4m{}\033[0m", name);
    } else {
        fmt::println("\033[4m{}\033[0m: {}", name, fullName);
    }

    if (description.length() <= maxWidth + indent) {
        fmt::println("    {}", description);
        return;
    }

    const std::vector<std::string_view> words = utils::strings::split(description, ' ');
    std::vector<std::string_view> line{"   "};
    std::uint32_t lineLength = 1;

    for (const auto &word : words) {
        if (lineLength + word.length() + 1 > maxWidth) {
            fmt::println("{}", fmt::join(line, ""));
            line.clear();
            line.push_back("    ");
            line.push_back(word);
            lineLength = 2 + word.length();
        } else {
            line.push_back(" ");
            line.push_back(word);
            lineLength += word.length() + 1;
        }
    }

    if (!line.empty()) {
        fmt::println("{}", fmt::join(line, ""));
    }
}
} // namespace

/* Parse the command line argument and fill a struct */
CLIArgs getArgs(int argc, char **argv) {
    fmt::print(std::cerr, "Submitted parameters:\n");
    for (size_t i = 0; i < argc; ++i) {
        // NOLINTNEXTLINE(*-pointer-arithmetic)
        fmt::print(std::cerr, "'{}' ", argv[i]);
    }
    fmt::println(std::cerr, "");
    fmt::println(std::cerr, "Version information: {}", VERSION);

    // Print current working directory
    fmt::println(std::cerr, "Current working directory: {}", std::filesystem::current_path());

    auto [options, args] = getOptions();

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help") > 0) {
            printUsage(options);
            std::exit(EXIT_SUCCESS);
        }

        if (result.count("version") > 0) {
            // Version has already been printed at this point. Just return.
            std::exit(EXIT_SUCCESS);
        }

        if (result.count("list-algorithms") > 0) {
            fmt::print("Available algorithms:\n");
            for (const auto algorithm : AlgorithmType::allAlgorithms) {
                const AlgorithmType t(algorithm);
                printAlgorithm(t.shortName(), t.fullName(), t.description());
            }
            std::exit(EXIT_SUCCESS);
        }

        if (result.count("list-modular-algorithms") > 0) {
            fmt::print("Available modular algorithms:\n");
            for (const auto algorithm : ModularAlgorithmType::allAlgorithms) {
                const ModularAlgorithmType t(algorithm);
                printAlgorithm(t.shortName(), t.fullName(), t.description());
            }
            std::exit(EXIT_SUCCESS);
        }

        if (result.count("list-modular-multi-algorithm-behaviour") > 0) {
            for (const auto behaviour : MultiAlgorithmBehaviour::allBehaviours) {
                const MultiAlgorithmBehaviour t(behaviour);
                printAlgorithm(t.shortName(), "", t.description());
            }
            std::exit(EXIT_SUCCESS);
        }

        if (result.count("input") <= 0 || result.count("output") <= 0) {
            std::cerr << "--input and --output are mandatory arguments\n";
            printUsage(options);
            std::exit(EXIT_FAILURE);
        }

        try {
            auto algsRange = result["algorithm"].as<std::vector<std::string>>()
                             | std::views::transform([](const std::string &name) {
                                   return AlgorithmType::parse(name);
                               });
            args.algorithms = std::vector<AlgorithmType>(algsRange.begin(), algsRange.end());

            args.algorithm = args.algorithms.front();
            args.modularAlgorithm =
                    ModularAlgorithmType::parse(result["modular-algorithm"].as<std::string>());
            args.outputFormat =
                    ScheduleOutputFormat::parse(result["output-format"].as<std::string>());
        } catch (const std::exception &e) {
            std::cout << "Unrecognized argument for the algorithm type\n";
            printUsage(options);
            std::exit(EXIT_FAILURE);
        }

        try {
            args.multiAlgorithmBehaviour = MultiAlgorithmBehaviour::parse(
                    result["modular-multi-algorithm-behaviour"].as<std::string>());
        } catch (const std::exception &e) {
            fmt::println(std::cerr,
                         "Unrecognized argument '{}' for the multi algorithm behaviour",
                         result["modular-multi-algorithm-behaviour"].as<std::string>());
        }

        for (size_t i = 0, verboseCount = result.count("verbose"); i < verboseCount; ++i) {
            utils::Logger::setVerbosity(increaseVerbosity(args.verbose));
        }

        args.inputFile = result["input"].as<std::string>();
        args.outputFile = result["output"].as<std::string>();
        args.maintPolicyFile = result["maintenance"].as<std::string>();
        args.productivityWeight = result["productivity"].as<double>();
        args.flexibilityWeight = result["flexibility"].as<double>();
        args.tieWeight = result["tie"].as<double>();
        args.timeOut = std::chrono::milliseconds(result["time-out"].as<std::int64_t>());
        args.maxIterations = result["max-iterations"].as<std::uint64_t>();
        args.maxPartialSolutions = result["max-partial"].as<std::uint32_t>();
        args.sequenceFile = result["sequence-file"].as<std::string>();

        if (result["modular-store-bounds"].count() > 0) {
            args.modularOptions.storeBounds = true;
        }

        if (result["modular-store-sequence"].count() > 0) {
            args.modularOptions.storeSequence = true;
        }

        if (result["modular-no-self-bounds"].count() > 0) {
            args.modularOptions.noSelfBounds = true;
        }

        args.modularOptions.maxIterations = result["modular-max-iterations"].as<std::uint64_t>();
        args.modularOptions.timeOut =
                std::chrono::milliseconds(result["modular-time-out"].as<std::int64_t>());

        args.shopType = ShopType::parse(result["shop-type"].as<std::string>());
        args.explorationType =
                DDExplorationType::parse(result["exploration-type"].as<std::string>());

        fmt::println("These are the parsed parameters:");
        for (const auto &kv : result) {
            fmt::println(std::cerr, "- {}: {}", kv.key(), kv.value());
        }

    } catch (const cxxopts::exceptions::exception &e) {
        fmt::print(std::cerr, "Error parsing arguments:\n{}\n", e.what());
        printUsage(options);
        std::exit(EXIT_FAILURE);
    }

    return args;
}

} // namespace fms::cli

// NOLINTEND(concurrency-mt-unsafe)
