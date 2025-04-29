#include "runner.hpp"

#include <fms/scheduler.hpp>

#include <filesystem>
#include <fmt/format.h>

using namespace fms;
using namespace fms::solvers;

problem::FORPFSSPSDXmlParser TestUtils::checkArguments(cli::CLIArgs &args,
                                                       std::string_view fileName) {
    namespace fs = std::filesystem;

    auto execDirectory = std::filesystem::current_path();
    fs::path filePath(execDirectory / fileName);

    if (!args.maintPolicyFile.empty()) {
        fs::path maintFile(execDirectory / args.maintPolicyFile);
        args.maintPolicyFile = maintFile.string();
        if (!fs::exists(args.maintPolicyFile)) {
            throw std::runtime_error(fmt::format("Maintenance policy file does not exist: {}",
                                                 args.maintPolicyFile));
        }
    }

    if (!fs::exists(filePath)) {
        throw fs::filesystem_error(fmt::format("File does not exist: {}", filePath.string()),
                                   std::make_error_code(std::errc::no_such_file_or_directory));
    }

    args.inputFile = filePath.string();
    return problem::FORPFSSPSDXmlParser(args.inputFile);
}

std::vector<PartialSolution> TestUtils::runShop(cli::CLIArgs &args, std::string_view fileName) {
    auto [solutions, instance, json] = runShopFullDetails(args, fileName);
    return solutions;
}

std::tuple<std::vector<PartialSolution>, fms::problem::Instance, nlohmann::json>
TestUtils::runShopFullDetails(cli::CLIArgs &args, std::string_view fileName) {
    auto parser = checkArguments(args, fileName);
    auto instance = Scheduler::loadFlowShopInstance(args, parser);
    auto [solutions, json] = Scheduler::runAlgorithm(instance, args);
    return {std::move(solutions), std::move(instance), std::move(json)};
}

std::tuple<std::vector<fms::solvers::ProductionLineSolution>, nlohmann::json>
TestUtils::runLine(cli::CLIArgs &args, std::string_view fileName) {
    auto instance = checkArguments(args, fileName).createProductionLine(args.shopType);
    return Scheduler::runAlgorithm(instance, args);
}
