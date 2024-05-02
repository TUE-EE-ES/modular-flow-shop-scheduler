#include "runner.h"
#include "fmsscheduler.h"

#include <filesystem>
#include <fmt/format.h>

FORPFSSPSDXmlParser TestUtils::checkArguments(commandLineArgs &args, std::string_view fileName) {
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
    return FORPFSSPSDXmlParser(args.inputFile);
}

std::vector<PartialSolution> TestUtils::runShop(commandLineArgs &args, std::string_view fileName) {
    auto parser = checkArguments(args, fileName);
    auto instance = FmsScheduler::loadFlowShopInstance(args, parser);
    return std::get<0>(FmsScheduler::runAlgorithm(instance, args));
}

std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
TestUtils::runLine(commandLineArgs &args, std::string_view fileName) {

    auto instance = checkArguments(args, fileName).createProductionLine(args.shopType);
    return FmsScheduler::runAlgorithm(instance, args);
}
