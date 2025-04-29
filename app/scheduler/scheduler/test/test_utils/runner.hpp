#ifndef TESTS_UTILS_RUNNER_HPP
#define TESTS_UTILS_RUNNER_HPP

#include <tuple>
#include <vector>

#include <fms/scheduler.hpp>
#include <fms/solvers/partial_solution.hpp>
#include <fms/cli/command_line.hpp>

namespace TestUtils {

fms::problem::FORPFSSPSDXmlParser checkArguments(fms::cli::CLIArgs &args,
                                                 std::string_view fileName);

std::vector<fms::solvers::PartialSolution> runShop(fms::cli::CLIArgs &args,
                                                   std::string_view fileName);
std::tuple<std::vector<fms::solvers::PartialSolution>, fms::problem::Instance, nlohmann::json>
runShopFullDetails(fms::cli::CLIArgs &args, std::string_view fileName);

std::tuple<std::vector<fms::solvers::ProductionLineSolution>, nlohmann::json>
runLine(fms::cli::CLIArgs &args, std::string_view fileName);

void checkRunRequirements(fms::cli::CLIArgs &args, const std::string &fileName);

} // namespace TestUtils

#endif // TESTS_UTILS_RUNNER_HPP
