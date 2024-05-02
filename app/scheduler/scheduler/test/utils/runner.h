#ifndef UTILS_RUNNER_H
#define UTILS_RUNNER_H

#include <fmsscheduler.h>
#include <partialsolution.h>
#include <utils/commandLine.h>

#include <filesystem>

namespace TestUtils {

FORPFSSPSDXmlParser checkArguments(commandLineArgs &args, std::string_view fileName);

std::vector<PartialSolution> runShop(commandLineArgs &args, std::string_view fileName);

std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
runLine(commandLineArgs &args, std::string_view fileName);

void checkRunRequirements(commandLineArgs &args, const std::string &fileName);

} // namespace TestUtils

#endif // UTILS_RUNNER_H