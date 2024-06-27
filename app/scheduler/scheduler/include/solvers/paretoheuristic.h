#ifndef PARETOHEURISTIC_H
#define PARETOHEURISTIC_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "delayGraph/delayGraph.h"
#include "partialsolution.h"
#include "schedulingoption.h"

#include <utility>
#include <vector>

namespace algorithm {

class ParetoHeuristic
{

public:
    static std::vector<PartialSolution> solve(FORPFSSPSD::Instance & problemInstance, const commandLineArgs & args);

    /* Algorithmic implementations */
    static std::vector<PartialSolution>
    scheduleOneOperation(DelayGraph::delayGraph &dg,
                         const FORPFSSPSD::Instance &problem,
                         const std::vector<PartialSolution> &currentSolutions,
                         const DelayGraph::vertex &eligibleOperation,
                         unsigned int maximumPartialSolutions);

    static std::pair<delay, unsigned int> countOperations(DelayGraph::delayGraph &dg,
                                                          const PartialSolution &ps);

    static std::pair<delay, unsigned int>
    countOperations(DelayGraph::delayGraph &dg, const PartialSolution &ps, const option &c);

    static delay determine_smallest_deadline(const DelayGraph::vertex &v);
};
}
#endif // PARETOHEURISTIC_H
