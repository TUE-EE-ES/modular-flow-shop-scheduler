#ifndef FMS_SCHEDULER_H
#define FMS_SCHEDULER_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/production_line.hpp"
#include "FORPFSSPSD/xmlParser.h"
#include "partialsolution.h"
#include "solvers/production_line_solution.hpp"
#include "utils/commandLine.h"

#include "pch/containers.hpp"

class FmsScheduler {
public:
    struct ErrorStrings {
        static constexpr auto kScheduler = "scheduler";
        static constexpr auto kNoSolution = "no-solution";
    };

    static void compute(commandLineArgs &args);

    static FORPFSSPSD::Instance loadFlowShopInstance(commandLineArgs &args,
                                                     FORPFSSPSDXmlParser &parser);

    static std::pair<bool, std::vector<delay>>
    checkConsistency(const FORPFSSPSD::Instance &flowshop);

    /**
     * @brief Runs the selected algorithm as provided by commandLineArgs::algorithm. Note only
     *        algorithms that return a PartialSolution can be run with this method otherwise it
     *        throws a std::runtime_error. This method **does not** support modular instances.
     *
     *        If you want to solve a modular problem use FmsScheduler::solve instead.
     * @param flowShopInstance Instance of the flow-shop problem
     * @param args Parsed command line arguments
     * @param iteration Number of the iteration to be run. This is used by some algorithms that 
     *        behave differently depending on the iteration, e.g. to generate counterexamples.
     * @return Solutions found with the selected algorithm. Note that some algorithms only return a
     *         single solution
     */
    [[nodiscard]] static std::tuple<std::vector<PartialSolution>, nlohmann::json>
    runAlgorithm(FORPFSSPSD::Instance &flowShopInstance,
                 const commandLineArgs &args,
                 std::uint64_t iteration = 0);

    [[nodiscard]] static std::tuple<std::vector<PartialSolution>, nlohmann::json>
    runAlgorithm(FORPFSSPSD::Module &flowShopInstance,
                 const commandLineArgs &args,
                 std::uint64_t iteration = 0);

    [[nodiscard]] static std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
    runAlgorithm(FORPFSSPSD::ProductionLine &problemInstance, const commandLineArgs &args);

    template <typename T> struct PrintMe;

    template <typename Problem>
    static void solveAndSave(Problem &problemInstance, commandLineArgs &args) {
        nlohmann::json data = initializeData(args);
        data["jobs"] = problemInstance.getNumberOfJobs();
        data["machines"] = problemInstance.getNumberOfMachines();

        // Get the type of the solution
        using OutputT = decltype(runAlgorithm(std::declval<Problem &>(),
                                              std::declval<const commandLineArgs &>()));
        using SolutionT = std::tuple_element_t<0, OutputT>::value_type;
        std::optional<SolutionT> bestSolution;

        try {
            const auto start = FMS::getCpuTime();
            const auto [solutions, dataRun] = runAlgorithm(problemInstance, args);
            const auto diff = FMS::getCpuTime() - start;
            const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();

            fmt::print(FMT_COMPILE("Solving {} finished in {}ms.\nSolving took {}ms per job.\n"),
                       problemInstance.getProblemName(),
                       time,
                       time / problemInstance.getNumberOfJobs());
            const auto itBest = getBestSolution(solutions);
            if (itBest != solutions.end()) {
                bestSolution = *itBest;
            } else if (data["error"].empty()) {
                data["error"] = ErrorStrings::kNoSolution;
            }

            addData(data, dataRun, bestSolution, time);

        } catch (FmsSchedulerException &e) {
            data["error"] = ErrorStrings::kScheduler;
            LOG(LOGGER_LEVEL::FATAL, fmt::format(FMT_COMPILE("Error: {}"), e.what()));
        }

        saveData(problemInstance, bestSolution, args, data);
    }

private:
    template <typename ProblemT, typename SolutionT>
    static void saveData(ProblemT problem,
                         std::optional<SolutionT> solution,
                         const commandLineArgs &args,
                         nlohmann::json data) {
        if (solution) {
            saveSolution(*solution, args, problem, data);
        }

        if (args.outputFormat == ScheduleOutputFormat::JSON) {
            saveJSONFile(data, args);
        } else if (args.outputFormat == ScheduleOutputFormat::CBOR) {
            saveCBORFile(data, args);
        }
    }

    static void saveSolution(const PartialSolution &solution,
                             const commandLineArgs &args,
                             const FORPFSSPSD::Instance &problem,
                             nlohmann::json &data);

    static void saveSolution(const FMS::ProductionLineSolution &solution,
                             const commandLineArgs &args,
                             const FORPFSSPSD::ProductionLine &problem,
                             nlohmann::json &data);

    [[nodiscard]] static nlohmann::json initializeData(const commandLineArgs &args);

    template <typename Solution>
    static void addData(nlohmann::json &data,
                        const nlohmann::json &dataRun,
                        const Solution &bestSolution,
                        const std::size_t totalTime) {
        if (!dataRun.is_null()) {
            data.update(dataRun);
        }
        data["totalTime"] = totalTime;

        if (bestSolution) {
            const auto minMakespan = bestSolution->getMakespan();
            const auto bestId = bestSolution->getId();
            fmt::print("Minimum makespan recorded is: {} for partial solution with ID {}\n",
                       minMakespan,
                       bestId);
            data["solved"] = true;
            data["minMakespan"] = minMakespan;
            data["bestSolution"] = bestId;
        } else {
            fmt::print("No solution found\n");
        }

        if (const auto it = data.find("iterations"); it != data.end()) {
            fmt::print("Total iterations: {}\n", it->get<uint32_t>());
        }
    }

    static void saveJSONFile(const nlohmann::json &data, const commandLineArgs &args);

    static void saveCBORFile(const nlohmann::json &data, const commandLineArgs &args);

    template <typename Solution>
    static auto getBestSolution(const std::vector<Solution> &solutions) {
        return std::min_element(
                solutions.begin(), solutions.end(), [](const auto &lhs, const auto &rhs) {
                    return lhs.getMakespan() < rhs.getMakespan();
                });
    }

    static void computeShop(commandLineArgs &args, FORPFSSPSDXmlParser parser);

    static void computeModular(commandLineArgs &args, FORPFSSPSDXmlParser parser);
};
#endif /* FMS_SCHEDULER_H */
