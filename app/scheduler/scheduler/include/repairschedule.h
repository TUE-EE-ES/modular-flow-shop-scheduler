//#ifndef MAINTENANCEHEURISTIC_H
//#define MAINTENANCEHEURISTIC_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "longest_path.h"

#include "pch/containers.hpp"

#include <memory>

namespace algorithm {

class RepairSchedule
{
public:
    static std::tuple<PartialSolution, DelayGraph::delayGraph>
    repairScheduleOffline(const FORPFSSPSD::Instance &problemInstance,
                          DelayGraph::delayGraph &dg,
                          PartialSolution solution,
                          FORPFSSPSD::operation eligibleOperation,
                          std::vector<delay> &ASAPST);

    static std::tuple<PartialSolution, DelayGraph::delayGraph>
    insertRepair(const FORPFSSPSD::Instance &problemInstance,
                 DelayGraph::delayGraph &dg,
                 PartialSolution solution,
                 FORPFSSPSD::operation eligibleOperation,
                 std::vector<delay> &ASAPST,
                 const std::vector<FORPFSSPSD::operation> &ops,
                 DelayGraph::Edges::difference_type start);

    static std::tuple<PartialSolution, DelayGraph::delayGraph>
    removeRepair(const FORPFSSPSD::Instance &problemInstance,
                 DelayGraph::delayGraph &dg,
                 PartialSolution solution,
                 FORPFSSPSD::operation eligibleOperation,
                 std::vector<delay> &ASAPST,
                 std::vector<FORPFSSPSD::operation> ops,
                 std::size_t start,
                 std::size_t end,
                 bool afterLast = true);

    static std::pair<FORPFSSPSD::JobId, DelayGraph::Edges::difference_type>
    findSecondToLastFirstPass(const FORPFSSPSD::Instance &problemInstance,
                              DelayGraph::delayGraph &dg,
                              const PartialSolution &solution,
                              FORPFSSPSD::MachineId machine,
                              DelayGraph::Edges::difference_type start);

    static FORPFSSPSD::JobId
    findLastCommittedSecondPass(const FORPFSSPSD::Instance &problemInstance,
                                DelayGraph::delayGraph &dg,
                                const PartialSolution &solution,
                                FORPFSSPSD::MachineId machine,
                                int start);

    static LongestPathResult
    recomputeSchedule(const FORPFSSPSD::Instance &problemInstance,
                      PartialSolution &schedule,
                      const FORPFSSPSD::MaintenancePolicy &maintPolicy,
                      DelayGraph::delayGraph &dg,
                      const DelayGraph::Edges &inputEdges,
                      std::vector<delay> &ASAPST);
};
} // namespace algorithm

//#endif // MAINTENANCEHEURISTIC_H