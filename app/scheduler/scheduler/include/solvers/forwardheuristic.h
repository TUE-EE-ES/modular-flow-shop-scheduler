#ifndef FORWARDHEURISTIC_H
#define FORWARDHEURISTIC_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "delay.h"
#include "delayGraph/delayGraph.h"
#include "delayGraph/edge.h"
#include "longest_path.h"
#include "partialsolution.h"
#include "utils/commandLine.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace algorithm {

class ForwardHeuristic
{

public:
    static PartialSolution solve(FORPFSSPSD::Instance & problemInstance, const commandLineArgs & args);
    static DelayGraph::Edges
    createInitialSequence(const FORPFSSPSD::Instance &problemInstance,
                          FORPFSSPSD::MachineId reEntrantMachine);

    static std::pair<DelayGraph::edge, std::vector<option>>
    createOptions(const DelayGraph::delayGraph &dg,
                  const FORPFSSPSD::Instance &problem,
                  const PartialSolution &solution,
                  const DelayGraph::vertex &eligibleOperation,
                  FORPFSSPSD::MachineId reEntrantMachineId);

    static std::vector<std::pair<PartialSolution, option>>
    evaluate_option_feasibility(DelayGraph::delayGraph &dg,
                                const FORPFSSPSD::Instance &problem,
                                const PartialSolution &solution,
                                const std::vector<option> &options,
                                const std::vector<delay> &ASAPTimes,
                                FORPFSSPSD::MachineId reEntrantMachine);

    static std::optional<std::pair<PartialSolution, option>>
    evaluate_option_feasibility(DelayGraph::delayGraph &dg,
                                              const FORPFSSPSD::Instance &problem,
                                              const PartialSolution &solution,
                                              const option &options,
                                              const std::vector<delay> &ASAPTimes,
                                              FORPFSSPSD::MachineId reEntrantMachine);

    /* Algorithmic implementations */

    static PartialSolution scheduleOneOperation(DelayGraph::delayGraph &dg,
                                                FORPFSSPSD::Instance &,
                                                const PartialSolution &current_solution,
                                                const FORPFSSPSD::operation &eligibleOperation,
                                                const commandLineArgs &args);

    static delay determine_smallest_deadline(const DelayGraph::vertex &v);
    /* add edges for the interleaving and validate whether the bound still hold; returns a negative cycle if the resulting graph contains one (or a list of infeasible edges) */
    static LongestPathResult
    validateInterleaving(DelayGraph::delayGraph &dg,
                         const FORPFSSPSD::Instance &problem,
                         const DelayGraph::Edges &InputEdges,
                         std::vector<delay> &ASAPST,
                         const DelayGraph::VerticesCRef &sources,
                         const DelayGraph::VerticesCRef &window);

    static std::pair<delay, unsigned int>
    computeFutureAvgProductivy(DelayGraph::delayGraph &dg,
                               const std::vector<delay> &ASAPST,
                               const PartialSolution &ps,
                               FORPFSSPSD::MachineId reEntrantMachineId);

    /**
     * @brief Ranks the given solutions and returns the index of the last one. Note that the
     *        solutions are not sorted.
     *
     * @param solutions Solutions to rank
     * @param ASAPTimes Start times of sequence before creating new options.
     * @param args Command line arguments. The user can select the exact weights of the ranking.
     * @param reEntrantMachine ID of the re-entrant machine whose operations are going to be ranked.
     * @return Index of the best solution and nullopt if no solution was found.
     */
    static std::optional<std::size_t> rankSolutions(
            std::vector<
                    std::tuple<PartialSolution, option, std::shared_ptr<DelayGraph::delayGraph>>>
                    &solutions,
            PathTimes &ASAPTimes,
            FORPFSSPSD::MachineId reEntrantMachine,
            const commandLineArgs &args);

    

        /**
     * @brief Same as rankSolutions above bur ranks only based on ASAP requirements
     *
     */
    static std::optional<std::size_t> rankSolutionsASAP(
        std::vector<std::tuple<PartialSolution, option, std::shared_ptr<DelayGraph::delayGraph>>>
                &solutions);

    /**
     * @brief Find all feasible insertion points and their rank.
     *
     * @param dg Current delay graph
     * @param problem Problem instance
     * @param eligibleOperation Operation to insert
     * @param solution Current solution
     * @param args Command line arguments
     * @return std::tuple<std::vector<PartialSolution>, std::size_t> Vector of feasible solutions
     * and the index of the best one.
     */
    static std::tuple<
            std::vector<std::tuple<PartialSolution, std::shared_ptr<DelayGraph::delayGraph>>>,
            std::optional<std::size_t>>
    getFeasibleOptions(DelayGraph::delayGraph &dg,
                       const FORPFSSPSD::Instance &problem,
                       const DelayGraph::vertex &eligibleOperation,
                       const PartialSolution &solution,
                       const commandLineArgs &args);
};
} // namespace algorithm
#endif // FORWARDHEURISTIC_H
