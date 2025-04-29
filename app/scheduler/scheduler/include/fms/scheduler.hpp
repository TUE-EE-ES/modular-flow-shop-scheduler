#ifndef FMS_SCHEDULER_HPP
#define FMS_SCHEDULER_HPP

#include "fms/cli/command_line.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/production_line.hpp"
#include "fms/problem/xml_parser.hpp"
#include "fms/solvers/partial_solution.hpp"
#include "fms/solvers/production_line_solution.hpp"
#include "fms/solvers/solver.hpp"
#include "fms/solvers/solver_data.hpp"

namespace fms {

class Scheduler {
public:
    struct ErrorStrings {
        static constexpr auto kScheduler = "scheduler";
        static constexpr auto kNoSolution = "no-solution";
    };

    static void compute(cli::CLIArgs &args);

    static problem::Instance loadFlowShopInstance(cli::CLIArgs &args,
                                                  problem::FORPFSSPSDXmlParser &parser);

    /**
     * @brief Checks that the flow shop is consistent
     * @param flowshop Flowshop whose consistency to check
     * @return std::pair<bool, std::vector<delay>> The first element is true when the initial graph
     * is consistent, false otherwise. The second element is a vector of the earliest possible
     * starting times of operations given the initial constraints.
     */
    static std::pair<bool, std::vector<delay>> checkConsistency(const problem::Instance &flowshop);

    /**
     * @brief Runs the selected algorithm as provided by CLIArgs::algorithm.
     * @param flowShopInstance Instance of the flow-shop problem
     * @param args Parsed command line arguments
     * @param iteration Number of the iteration to be run. This is used by some algorithms that
     *        behave differently depending on the iteration, e.g. to generate counterexamples.
     * @return Solutions found with the selected algorithm. Note that some algorithms only return a
     *         single solution
     */
    [[nodiscard]] static std::tuple<solvers::Solutions, nlohmann::json>
    runAlgorithm(problem::Instance &flowShopInstance,
                 const cli::CLIArgs &args,
                 std::uint64_t iteration = 0);

    [[nodiscard]] static std::tuple<solvers::Solutions, nlohmann::json>
    runAlgorithm(const problem::ProductionLine &line,
                 problem::Module &flowShopInstance,
                 const cli::CLIArgs &args,
                 std::uint64_t iteration = 0);

    [[nodiscard]] static std::tuple<solvers::ProductionLineSolutions, nlohmann::json>
    runAlgorithm(problem::ProductionLine &problemInstance, const cli::CLIArgs &args);

    /**
     * @brief Runs the selected resumable solver as provided by CLIArgs::algorithm.
     * @param problem Instance of the flow-shop problem
     * @param args Parsed command line arguments
     * @param solverData Scheduler-dependent data that can be used to resume the solver.
     * @return Solutions found by the solver, information data about the run and the solver data
     * that can be used to resume the solver later.
     */
    [[nodiscard]] static std::tuple<solvers::Solutions, nlohmann::json, solvers::SolverDataPtr>
    runResumable(problem::Instance &problem,
                 const cli::CLIArgs &args,
                 solvers::SolverDataPtr solverData);

    [[nodiscard]] static std::tuple<solvers::Solutions, nlohmann::json, solvers::SolverDataPtr>
    runResumable(problem::Module &problem,
                 const cli::CLIArgs &args,
                 solvers::SolverDataPtr solverData);

    template <typename Problem>
    static void solveAndSave(Problem &problemInstance, cli::CLIArgs &args) {
        nlohmann::json data = initializeData(args);
        data["jobs"] = problemInstance.getNumberOfJobs();
        data["machines"] = problemInstance.getNumberOfMachines();

        // Get the type of the solution
        using OutputT = decltype(runAlgorithm(std::declval<Problem &>(),
                                              std::declval<const cli::CLIArgs &>()));
        using SolutionT = std::tuple_element_t<0, OutputT>::value_type;
        std::optional<SolutionT> bestSolution;

        try {
            const auto start = utils::time::getCpuTime();
            const auto [solutions, dataRun] = runAlgorithm(problemInstance, args);
            const auto diff = utils::time::getCpuTime() - start;
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

            addData(data, dataRun, bestSolution, time, problemInstance);

        } catch (FmsSchedulerException &e) {
            data["error"] = ErrorStrings::kScheduler;
            LOG_C(FMT_COMPILE("Error: {}"), e.what());
        }

        saveData(problemInstance, bestSolution, args, data);
    }

    static cli::AlgorithmType getAlgorithm(const problem::ModuleId moduleId,
                                           const std::size_t numAlgorithms,
                                           const std::size_t numModules,
                                           const cli::CLIArgs &args);

private:
    template <typename ProblemT, typename SolutionT>
    static void saveData(ProblemT problem,
                         std::optional<SolutionT> solution,
                         const cli::CLIArgs &args,
                         nlohmann::json data) {
        if (solution) {
            saveSolution(*solution, problem, data);
        }

        if (args.outputFormat == cli::ScheduleOutputFormat::JSON) {
            saveJSONFile(data, args);
        } else if (args.outputFormat == cli::ScheduleOutputFormat::CBOR) {
            saveCBORFile(data, args);
        } else {
            LOG_E("Output format not implemented");
        }
    }

    static void saveSolution(const solvers::PartialSolution &solution,
                             const problem::Instance &problem,
                             nlohmann::json &data);

    static void saveSolution(const solvers::ProductionLineSolution &solution,
                             const problem::ProductionLine &problem,
                             nlohmann::json &data);

    [[nodiscard]] static nlohmann::json initializeData(const cli::CLIArgs &args);

    template <typename Solution, typename Problem>
    static void addData(nlohmann::json &data,
                        const nlohmann::json &dataRun,
                        const Solution &bestSolution,
                        const std::size_t totalTime,
                        const Problem &instance) {
        if (!dataRun.is_null()) {
            data.update(dataRun);
        }
        data["totalTime"] = totalTime;
        delay minMakespan = 0;

        if (bestSolution) {
            // Check if the type is a ProductionLineSolution
            if constexpr (std::is_same_v<Solution,
                                         std::optional<solvers::ProductionLineSolution>>) {
                minMakespan = bestSolution->getMakespan();
            } else {
                minMakespan = bestSolution->getRealMakespan(instance);
            }
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

    static void saveJSONFile(const nlohmann::json &data, const cli::CLIArgs &args);

    static void saveCBORFile(const nlohmann::json &data, const cli::CLIArgs &args);

    template <typename Solution>
    static auto getBestSolution(const std::vector<Solution> &solutions) {
        return std::min_element(
                solutions.begin(), solutions.end(), [](const auto &lhs, const auto &rhs) {
                    return lhs.getMakespan() < rhs.getMakespan();
                });
    }

    static void computeShop(cli::CLIArgs &args, problem::FORPFSSPSDXmlParser parser);

    static void computeModular(cli::CLIArgs &args, problem::FORPFSSPSDXmlParser parser);
};

} // namespace fms

#endif /* FMS_SCHEDULER_HPP */
