#ifndef BRANCHBOUND_H
#define BRANCHBOUND_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"

class BranchBoundNode;

namespace algorithm {

class BranchBound
{

public:
    static PartialSolution solve(FORPFSSPSD::Instance &problemInstance, const commandLineArgs & args);
    static DelayGraph::Edges
    create_initial_sequence(const FORPFSSPSD::Instance &problemInstance,
                            unsigned int reentrant_machine);
    static std::vector<PartialSolution>
    ranked(DelayGraph::delayGraph &dg,
           const FORPFSSPSD::Instance &problem,
           const std::vector<std::pair<PartialSolution, option>> &generationOfSolutions,
           const std::vector<delay> &ASAPTimes);

    /* Algorithmic implementations */
    static std::vector<PartialSolution>
    scheduleOneOperation(DelayGraph::delayGraph &dg,
                         const FORPFSSPSD::Instance &,
                         const PartialSolution &current_solution,
                         const DelayGraph::vertex &);

    static BranchBoundNode createStupidSchedule(FORPFSSPSD::Instance &problemInstance, FORPFSSPSD::MachineId reentrant_machine);
};
}
#endif // BRANCHBOUND_H
