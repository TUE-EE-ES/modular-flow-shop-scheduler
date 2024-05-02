//#ifndef MAINTENANCEHEURISTIC_H
//#define MAINTENANCEHEURISTIC_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "longest_path.h"

#include "pch/containers.hpp"

#include <memory>

namespace algorithm {

class MaintenanceHeuristic
{
public:
    static std::tuple<PartialSolution, DelayGraph::delayGraph>
    triggerMaintenance(DelayGraph::delayGraph dg,
                       const FORPFSSPSD::Instance &problemInstance,
                       FORPFSSPSD::MachineId machine,
                       const PartialSolution &solution,
                       const commandLineArgs &args);
    static std::tuple<PartialSolution, DelayGraph::delayGraph>
    triggerMaintenance(DelayGraph::delayGraph dg,
                       const FORPFSSPSD::Instance &problemInstance,
                       const PartialSolution &solution,
                       const option &eligibleOption,
                       const commandLineArgs &args);
    static std::tuple<PartialSolution, DelayGraph::delayGraph>
    triggerMaintenance(DelayGraph::delayGraph dg,
                       const FORPFSSPSD::Instance &problemInstance,
                       const PartialSolution &solution,
                       FORPFSSPSD::operation eligibleOperation,
                       FORPFSSPSD::operation nextOperation,
                       const commandLineArgs &args);

    static std::tuple<PartialSolution, DelayGraph::delayGraph>
    evaluateSchedule(const FORPFSSPSD::Instance &problemInstance,
                     DelayGraph::delayGraph &dg,
                     const PartialSolution &schedule,
                     const FORPFSSPSD::operation& eligibleOperation,
                     const FORPFSSPSD::operation& nextOperation,
                     const commandLineArgs &args);
    static std::pair<PartialSolution, DelayGraph::delayGraph>
    insertMaintenance(const FORPFSSPSD::Instance &problemInstance,
                      FORPFSSPSD::MachineId machine,
                      DelayGraph::delayGraph &dg,
                      const PartialSolution &schedule,
                      std::vector<delay> &ASAPST,
                      int i,
                      unsigned int actionID);
    static std::pair<long, long> fetchIdle(const FORPFSSPSD::Instance &problemInstance,
                                           FORPFSSPSD::MachineId machine,
                                           DelayGraph::delayGraph &dg,
                                           const PartialSolution &schedule,
                                           std::vector<delay> &ASAPST,
                                           std::vector<delay> &TLU,
                                           int i);
    static unsigned int checkInterval(std::pair<long,long> idle, const FORPFSSPSD::MaintenancePolicy& maintPolicy, const commandLineArgs &args);
    static LongestPathResult recomputeSchedule(const FORPFSSPSD::Instance &problemInstance,
                                               PartialSolution &schedule,
                                               const FORPFSSPSD::MaintenancePolicy &maintPolicy,
                                               DelayGraph::delayGraph &dg,
                                               const DelayGraph::Edges &InputEdges,
                                               std::vector<delay> &ASAPST,
                                               const DelayGraph::VerticesCRef &sources,
                                               const DelayGraph::VerticesCRef &window);
};

}

//#endif // MAINTENANCEHEURISTIC_H