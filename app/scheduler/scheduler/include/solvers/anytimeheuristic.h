#ifndef ANYTIMEHEURISTIC_H
#define ANYTIMEHEURISTIC_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "delay.h"
#include "delayGraph/vertex.h"
#include "longest_path.h"
#include "schedulingoption.h"

#include <cstddef>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace algorithm {

class AnytimeHeuristic
{
     /* Anytime counterpart of fowardheuristic's createOptions
     Gets the next possible option for inserting a particular job based on a decided 
     way to search the option space
     Currently just listed in order */
    static std::optional<std::pair<option,size_t>> getNextOption(std::vector<option> options, size_t index);

    /* Anytime counterpart of fowardheuristic's rankSolutions
     Gets the next possible option for inserting a particular job based on a decided 
     way to search the option space
     Currently just listed in order */

    static std::tuple<std::tuple<delay, delay, delay, delay, std::uint32_t, std::uint32_t>,
                      std::tuple<PartialSolution, delay, delay, std::uint32_t>>
    rankSolution(std::tuple<PartialSolution, option> solution,
                 DelayGraph::delayGraph &dg,
                 std::tuple<delay, delay, delay, delay, std::uint32_t, std::uint32_t> existingNorms,
                 std::tuple<PartialSolution, delay, delay, std::uint32_t> existingRank,
                 PathTimes &ASAPTimes,
                 FORPFSSPSD::MachineId reEntrantMachine,
                 const commandLineArgs &args);

    /* Algorithmic implementations */

    static PartialSolution scheduleOneOperation(DelayGraph::delayGraph &dg,
                                                FORPFSSPSD::Instance &,
                                                const PartialSolution &current_solution,
                                                const DelayGraph::vertex &,
                                                const commandLineArgs &args);

    static PartialSolution getSolution(DelayGraph::delayGraph &dg,
                                       const FORPFSSPSD::Instance &problem,
                                       const DelayGraph::vertex &eligibleOperation,
                                       const PartialSolution &solution,
                                       const commandLineArgs &args);

    static std::optional<std::pair<PartialSolution, option>>
    evaluateOption(DelayGraph::delayGraph &dg,
                   const FORPFSSPSD::Instance &problem,
                   const DelayGraph::vertex &eligibleOperation,
                   const PartialSolution &solution,
                   const option &o,
                   const DelayGraph::edge &lastPotentiallyFeasibleOption,
                   std::tuple<delay, delay, delay, delay, uint32_t, uint32_t> &existingNorms,
                   std::tuple<PartialSolution, delay, delay, uint32_t> &existingRank,
                   const commandLineArgs &args);

    DelayGraph::Edges getExitEdges();

public:
    static PartialSolution solve(FORPFSSPSD::Instance & problemInstance, const commandLineArgs & args);

};
}// namespace algorithm

#endif // ANYTIMEHEURISTIC_H