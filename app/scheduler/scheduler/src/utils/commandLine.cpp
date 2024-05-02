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

#include "utils/commandLine.h"

#include "utils/strings.hpp"
#include "versioning.h"

#include <algorithm>
#include <cstdlib>
#include <cxxopts.hpp>
#include <fmt/compile.h>
#include <fmt/ostream.h>
#include <iostream>

// NOLINTBEGIN(concurrency-mt-unsafe): This is only single threaded code

namespace {
std::tuple<cxxopts::Options, commandLineArgs> getOptions() {
    cxxopts::Options options("fms-scheduler", "A Heuristic based Constraint Scheduler");
    commandLineArgs args;

    // clang-format off
    options.add_options()
        ("h,help", "Show this help")
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
            cxxopts::value<std::string>()->default_value(std::string{args.algorithm.shortName()}))
        ("r,output-format", "Output format (operation|separation_buffer|json|cbor)",
            cxxopts::value<std::string>()->default_value(std::string{args.outputFormat.shortName()}))
        ("s,sequence-file", "Re-entrant machine operation sequence file",
            cxxopts::value<std::string>()->default_value(args.sequenceFile))
        ("max-iterations", "Maximum number of iterations that the algorithm should perform",
            cxxopts::value<std::uint64_t>()->default_value(std::to_string(args.maxIterations)))
        ("modular-algorithm", "Algorithm to use for modular scheduling (broadcast|cocktail)", 
            cxxopts::value<std::string>()->default_value(std::string{args.modularAlgorithm.shortName()}))
        ("modular-algorithm-option", "Extra options that can be passed to the modular algorithm.\n"
            "Accepted options are:\n - 'store-bounds': Store the bounds in the output JSON.\n"
            " - 'store-sequence': Store the sequence in the output JSON.\n"
            " - 'no-self-bounds': Do not store the bounds that a module sends in the module itself.",
            cxxopts::value<std::vector<std::string>>())
        ("shop-type", "Tell the SAG solution what type of shop it is solving.\n"
            "Accepted options are: 'flow','job' or 'fixedorder'",
            cxxopts::value<std::string>()->default_value(std::string{args.shopType.shortName()}))
        ("exploration-type", "Tell the DD solution what type of graph exploration technique it should use.\n"
            "Accepted options are: 'breadth','depth', 'best','static' or 'adaptive'",
            cxxopts::value<std::string>()->default_value(std::string{args.explorationType.shortName()}))
        ("list-algorithms",
            "List all available algorithms and exit")
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

} // namespace

using namespace FMS;

std::string_view AlgorithmType::fullName() const {
    switch (m_value) {
    case Value::BHCS:
        return "forward heuristic";
    case Value::MDBHCS:
        return "pareto heuristic";
    case Value::MIBHCS:
        return "maintenance aware forward heuristic";
    case Value::MISIM:
        return "simulate maintenance insertion at end of forward heuristic";
    case Value::ASAP:
        return "asap forward heuristic";
    case Value::MIASAP:
        return "maintenance aware asap forward heuristic";
    case Value::MIASAPSIM:
        return "simulate maintenance insertion at end of asap forward heuristic";
    case Value::NEH:
        return "MHEH flowshop scheduling heuristic";
    case Value::MINEH:
        return "maintenance aware MHEH flowshop scheduling heuristic";
    case Value::MINEHSIM:
        return "simulate maintenance insertion at end of MHEH flowshop scheduling heuristic";
    case Value::BRANCH_BOUND:
        return "branch & bound";
    case Value::GIVEN_SEQUENCE:
        return "given sequence";
    case Value::ANYTIME:
        return "anytime heuristic";
    case Value::ITERATED_GREEDY:
        return "iterated greedy solver";
    case Value::DD:
        return "Decision diagram";
    case Value::DDSeed:
        return "Decision diagram built around a seed solution";
    }

    return "";
}

std::string_view AlgorithmType::shortName() const {
    switch (m_value) {
    case Value::BHCS:
        return "bhcs";
    case Value::MDBHCS:
        return "mdbhcs";
    case Value::MIBHCS:
        return "mibhcs";
    case Value::MISIM:
        return "misim";
    case Value::ASAP:
        return "asap";
    case Value::MIASAP:
        return "miasap";
    case Value::MIASAPSIM:
        return "miasapsim";
    case Value::NEH:
        return "neh";
    case Value::MINEH:
        return "mineh";
    case Value::MINEHSIM:
        return "minehsim";
    case Value::BRANCH_BOUND:
        return "branch-bound";
    case Value::GIVEN_SEQUENCE:
        return "sequence";
    case Value::ANYTIME:
        return "anytime";
    case Value::ITERATED_GREEDY:
        return "iterated-greedy";
    case Value::DD:
        return "dd";
    case Value::DDSeed:
        return "ddseed";
    }

    return "";
}

AlgorithmType AlgorithmType::parse(std::string_view name) {
    const std::string lowerCase = toLower(name);

    for (const auto algorithm : allAlgorithms) {
        if (AlgorithmType(algorithm).shortName() == lowerCase) {
            return algorithm;
        }
    }

    throw std::runtime_error(fmt::format("Unknown algorithm type: {}", name));
}

ModularAlgorithmType ModularAlgorithmType::parse(const std::string &name) {
    const std::string lowerCase = toLower(name);

    for (const auto algorithm : allAlgorithms) {
        if (ModularAlgorithmType(algorithm).shortName() == lowerCase) {
            return algorithm;
        }
    }

    throw std::runtime_error(fmt::format("Unknown modular algorithm type: {}", name));
}

std::string_view ModularAlgorithmType::fullName() const {
    switch (m_value) {
    case Value::BROADCAST:
        return "broadcast";
    case Value::COCKTAIL:
        return "cocktail";
    case Value::BACKTRACK:
        return "backtrack";
    }

    return "";
}

std::string_view ModularAlgorithmType::shortName() const {
    switch (m_value) {
    case Value::BROADCAST:
        return "broadcast";
    case Value::COCKTAIL:
        return "cocktail";
    case Value::BACKTRACK:
        return "backtrack";
    }

    return "";
}

DDExplorationType DDExplorationType::parse(const std::string &name) {
    const std::string lowerCase = toLower(name);

    if (lowerCase == "breadth") {
        return DDExplorationType::BREADTH;
    }
    if (lowerCase == "depth") {
        return DDExplorationType::DEPTH;
    }
    if (lowerCase == "best") {
        return DDExplorationType::BEST;
    }
    if (lowerCase == "static") {
        return DDExplorationType::STATIC;
    }
    if (lowerCase == "adaptive") {
        return DDExplorationType::ADAPTIVE;
    }
    throw std::runtime_error("Unknown shop type: " + name);
}

std::string_view DDExplorationType::shortName() const {
    switch (m_value) {
    case Value::BREADTH:
        return "breadth";
    case Value::DEPTH:
        return "depth";
    case Value::BEST:
        return "best";
    case Value::STATIC:
        return "static";
    case Value::ADAPTIVE:
        return "adaptive";
    }

    return "";
}

ShopType ShopType::parse(const std::string &name) {
    const std::string lowerCase = toLower(name);

    if (lowerCase == "flow") {
        return ShopType::FLOWSHOP;
    }
    if (lowerCase == "job") {
        return ShopType::JOBSHOP;
    }
    if (lowerCase == "fixedorder") {
        return ShopType::FIXEDORDERSHOP;
    }

    throw std::runtime_error("Unknown shop type: " + name);
}

std::string_view ShopType::shortName() const {
    switch (m_value) {
    case Value::FLOWSHOP:
        return "flow";
    case Value::JOBSHOP:
        return "job";
    case Value::FIXEDORDERSHOP:
        return "fixedorder";
    }

    return "";
}

ScheduleOutputFormat ScheduleOutputFormat::parse(const std::string &name) {
    const std::string lowerCase = toLower(name);
    if (lowerCase == "operation") {
        return ScheduleOutputFormat::OPERATION_TIMES;
    }
    if (lowerCase == "separation_buffer") {
        return ScheduleOutputFormat::SEPARATION_AND_BUFFER;
    }
    if (lowerCase == "json") {
        return ScheduleOutputFormat::JSON;
    }
    if (lowerCase == "cbor") {
        return ScheduleOutputFormat::CBOR;
    }
    throw std::runtime_error("Unknown schedule output format: " + name);
}

std::string_view ScheduleOutputFormat::shortName() const {
    switch (m_value) {
    case Value::OPERATION_TIMES:
        return "operation";
    case Value::SEPARATION_AND_BUFFER:
        return "separation_buffer";
    case Value::JSON:
        return "json";
    case Value::CBOR:
        return "cbor";
    }

    return "";
}

/* Parse the command line argument and fill a struct */
commandLineArgs commandLine::getArgs(int argc, char **argv) noexcept {
    fmt::print(std::cerr, "Submitted parameters:\n");
    for (size_t i = 0; i < argc; ++i) {
        // NOLINTNEXTLINE(*-pointer-arithmetic)
        fmt::print(std::cerr, "'{}' ", argv[i]);
    }
    fmt::println(std::cerr, "");
    fmt::println(std::cerr, "Version {}", VERSION);

    auto [options, args] = getOptions();

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help") > 0) {
            printUsage(options);
            std::exit(EXIT_SUCCESS);
        }

        if (result.count("list-algorithms") > 0) {
            fmt::print("Available algorithms:\n");
            for (const auto algorithm : AlgorithmType::allAlgorithms) {
                const AlgorithmType t(algorithm);
                fmt::print("{}: {}\n", t.shortName(), t.fullName());
            }
            std::exit(EXIT_SUCCESS);
        }

        if (result.count("input") <= 0 || result.count("output") <= 0) {
            std::cerr << "--input and --output are mandatory arguments\n";
            printUsage(options);
            std::exit(EXIT_FAILURE);
        }

        try {
            args.algorithm = AlgorithmType::parse(result["algorithm"].as<std::string>());
            args.modularAlgorithm =
                    ModularAlgorithmType::parse(result["modular-algorithm"].as<std::string>());
            args.outputFormat =
                    ScheduleOutputFormat::parse(result["output-format"].as<std::string>());
        } catch (const std::exception &e) {
            std::cout << "Unrecognized argument for the algorithm type\n";
            printUsage(options);
            std::exit(EXIT_FAILURE);
        }

        for (size_t i = 0, verboseCount = result.count("verbose"); i < verboseCount; ++i) {
            Logger::setVerbosity(increaseVerbosity(args.verbose));
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

        if (result["modular-algorithm-option"].count() > 0) {
            auto modularArgs = result["modular-algorithm-option"].as<std::vector<std::string>>();
            std::ranges::transform(modularArgs, modularArgs.begin(), toLower);
            args.modularAlgorithmOption = modularArgs;
        }

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

// NOLINTEND(concurrency-mt-unsafe)