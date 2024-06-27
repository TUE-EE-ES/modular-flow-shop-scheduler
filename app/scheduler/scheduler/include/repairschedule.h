#ifndef REPAIRSCHEDULE_H
#define REPAIRSCHEDULE_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/operation.h"
#include "delayGraph/delayGraph.h"
#include "longest_path.h"
#include "partialsolution.h"

#include <cstddef>
#include <vector>

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

#endif // REPAIRSCHEDULE_H
